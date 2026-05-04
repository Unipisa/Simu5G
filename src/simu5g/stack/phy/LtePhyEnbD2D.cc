//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/phy/LtePhyEnbD2D.h"
#include "simu5g/stack/phy/packet/LteFeedbackPkt.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(LtePhyEnbD2D);

using namespace omnetpp;
using namespace inet;


void LtePhyEnbD2D::initialize(int stage)
{
    LtePhyEnb::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
        enableD2DCqiReporting_ = par("enableD2DCqiReporting");
}

void LtePhyEnbD2D::handleFeedbackPkt(UserControlInfo *lteinfo, LteAirFrame *frame)
{
    EV << "LtePhyEnbD2D::handleFeedbackPkt - received Feedback Packet with ID " << frame->getId() << endl;
    auto pktAux = check_and_cast<Packet *>(frame->decapsulate());
    auto header = pktAux->removeAtFront<LteFeedbackPkt>();

    *(pktAux->addTagIfAbsent<UserControlInfo>()) = *lteinfo;

    LteFeedbackDoubleVector fb;
    FeedbackRequest req = lteinfo->getFeedbackReq();
    fb.clear();
    fb = ulFbGen_->computeUlFeedback(lteinfo, frame);
    header->setLteFeedbackDoubleVectorUl(fb);

    if (!req.dlFeedbackFromUe) {
        LteChannelModel *channelModel = getChannelModel(lteinfo->getCarrierFrequency());
        if (channelModel == nullptr)
            throw cRuntimeError("LtePhyEnbD2D::handleFeedbackPkt - channelModel is null pointer");

        EV_DEBUG << "LtePhyEnbD2D::handleFeedbackPkt - computing DL CSI" << endl;
        int nRus = 0;
        TxMode txmode = req.txMode;
        FeedbackType type = req.type;
        RbAllocationType rbtype = req.rbAllocationType;
        std::map<Remote, int> antennaCws; // DAS functionality removed
        antennaCws[MACRO] = 1; // Default single antenna
        unsigned int numPreferredBand = cellInfo_->getNumPreferredBands();

        lteinfo->setTxPower(txPower_);
        lteinfo->setDirection(DL);
        std::vector<double> snr = channelModel->getSINR(frame, lteinfo);

        fb = lteFeedbackComputation_->computeFeedback(type, rbtype, txmode,
                antennaCws, numPreferredBand, nRus, snr,
                lteinfo->getSourceId());
        header->setLteFeedbackDoubleVectorDl(fb);

    }

    if (enableD2DCqiReporting_) {
        for (const auto& ueInfo : binder_->getUeList()) {
            MacNodeId peerId = ueInfo->id;
            if (peerId != lteinfo->getSourceId() && binder_->getD2DCapability(lteinfo->getSourceId(), peerId) && binder_->getNextHop(peerId) == nodeId_) {
                Coord peerCoord = ueInfo->phy->getCoord();
                fb = ulFbGen_->computeD2DFeedback(lteinfo, frame, peerId, peerCoord);
                header->setLteFeedbackDoubleVectorD2D(peerId, fb);
            }
        }
    }
    EV << "LtePhyEnbD2D::handleFeedbackPkt : Feedback Generated for nodeId: "
       << nodeId_ << " Fb size: " << fb.size()
       << " Carrier: " << lteinfo->getCarrierFrequency() << endl;

    pktAux->insertAtFront(header);
    delete lteinfo;
    send(pktAux, upperGateOut_);
}

void LtePhyEnbD2D::handleAirFrame(cMessage *msg)
{
    LteAirFrame *frame = static_cast<LteAirFrame *>(msg);
    UserControlInfo *lteInfo = new UserControlInfo(frame->getAdditionalInfo());

    EV << "LtePhyEnbD2D::handleAirFrame - received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    // Handle broadcast packet sent by another eNB
    if (lteInfo->getFrameType() == HANDOVERPKT) {
        EV << "LtePhyEnbD2D::handleAirFrame - received handover packet from another eNodeB. Ignore it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // Check if the air frame was sent on a correct carrier frequency
    GHz carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel *channelModel = getChannelModel(carrierFreq);
    if (channelModel == nullptr) {
        EV << "Received packet on carrier frequency not supported by this node. Delete it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // Check if the frame is for us ( MacNodeId matches or - if this is a multicast communication - enrolled in multicast group)
    if (lteInfo->getDestId() != nodeId_) {
        EV << "ERROR: Frame is not for us. Delete it." << endl;
        EV << "Packet Type: " << phyFrameTypeToA((LtePhyFrameType)lteInfo->getFrameType()) << endl;
        EV << "Frame MacNodeId: " << lteInfo->getDestId() << endl;
        EV << "Local MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    if (lteInfo->getPacketMulticastGroupId() != NODEID_NONE && !(binder_->isInMulticastGroup(nodeId_, lteInfo->getPacketMulticastGroupId()))) {
        EV << "Frame is for a multicast group, but we do not belong to that group. Delete the frame." << endl;
        EV << "Packet Type: " << phyFrameTypeToA((LtePhyFrameType)lteInfo->getFrameType()) << endl;
        EV << "Frame MacNodeId: " << lteInfo->getDestId() << endl;
        EV << "Local MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    /*
     * This could happen if the UE associates with a new master while it has
     * already scheduled a packet for the old master: the packet is in the air
     * while the UE changes master.
     * Event timing:      TTI x: packet scheduled and sent by the UE (tx time = 1ms)
     *                     TTI x+0.1: UE changes master
     *                     TTI x+1: packet from UE arrives at the old master
     */
    if (binder_->getNextHop(lteInfo->getSourceId()) != nodeId_) {
        EV << "WARNING: frame from a UE that is leaving this cell (handover): deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "Master MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    connectedNodeId_ = lteInfo->getSourceId();

    if (!binder_->nodeExists(connectedNodeId_) || !binder_->nodeExists(lteInfo->getDestId())) {
        // Either source or destination have left the simulation
        delete msg;
        return;
    }

    // Handle all control pkt
    if (handleControlPkt(lteInfo, frame))
        return; // If frame contains a control pkt no further action is needed

    // DAS removed - single antenna only
    bool result = channelModel->isReceptionSuccessful(frame, lteInfo);
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << (result ? "RECEIVED" : "NOT RECEIVED") << endl;

    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());

    // Here frame has to be destroyed since it is no longer useful
    delete frame;

    // Attach the decider result to the packet as control info
    auto lteInfoTag = pkt->addTagIfAbsent<UserControlInfo>();
    *lteInfoTag = *lteInfo;
    delete lteInfo;

    pkt->addTagIfAbsent<PhyReceptionInd>()->setDeciderResult(result);

    // Send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

} //namespace
