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

#include "stack/phy/LtePhyEnbD2D.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "common/LteCommon.h"
#include "stack/phy/das/DasFilter.h"

namespace simu5g {

Define_Module(LtePhyEnbD2D);

using namespace omnetpp;
using namespace inet;


void LtePhyEnbD2D::initialize(int stage)
{
    LtePhyEnb::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
        enableD2DCqiReporting_ = par("enableD2DCqiReporting");
}

void LtePhyEnbD2D::requestFeedback(UserControlInfo *lteinfo, LteAirFrame *frame, Packet *pktAux)
{
    EV << NOW << " LtePhyEnbD2D::requestFeedback " << endl;

    LteFeedbackDoubleVector fb;

    // Select the correct channel model according to the carrier frequency
    LteChannelModel *channelModel = getChannelModel(lteinfo->getCarrierFrequency());

    auto header = pktAux->removeAtFront<LteFeedbackPkt>();

    // Get UE position
    Coord sendersPos = lteinfo->getCoord();
    cellInfo_->setUePosition(lteinfo->getSourceId(), sendersPos);

    // Apply analog model (pathloss)
    // Get SNR for UL direction
    std::vector<double> snr;
    if (channelModel != nullptr)
        snr = channelModel->getSINR(frame, lteinfo);
    else
        throw cRuntimeError("LtePhyEnbD2D::requestFeedback - channelModel is null pointer. Abort");
    FeedbackRequest req = lteinfo->feedbackReq;
    // Feedback computation
    fb.clear();
    // Get number of RU
    int nRus = cellInfo_->getNumRus();
    TxMode txmode = req.txMode;
    FeedbackType type = req.type;
    RbAllocationType rbtype = req.rbAllocationType;
    std::map<Remote, int> antennaCws = cellInfo_->getAntennaCws();
    unsigned int numPreferredBand = cellInfo_->getNumPreferredBands();
    Direction dir = UL;
    while (dir != UNKNOWN_DIRECTION) {
        // For each RU the computation feedback function is called
        if (req.genType == IDEAL) {
            fb = lteFeedbackComputation_->computeFeedback(type, rbtype, txmode,
                    antennaCws, numPreferredBand, IDEAL, nRus, snr,
                    lteinfo->getSourceId());
        }
        else if (req.genType == REAL) {
            fb.resize(das_->getReportingSet().size());
            for (const auto& reporting : das_->getReportingSet())
            {
                fb[reporting].resize((int)txmode);
                fb[reporting][(int)txmode] =
                    lteFeedbackComputation_->computeFeedback(reporting, txmode,
                            type, rbtype, antennaCws[reporting], numPreferredBand,
                            REAL, nRus, snr, lteinfo->getSourceId());
            }
        }
        // The reports are computed only for the antennas in the reporting set
        else if (req.genType == DAS_AWARE) {
            fb.resize(das_->getReportingSet().size());
            for (const auto& reporting : das_->getReportingSet())
            {
                fb[reporting] = lteFeedbackComputation_->computeFeedback(reporting, type,
                        rbtype, txmode, antennaCws[reporting], numPreferredBand,
                        DAS_AWARE, nRus, snr, lteinfo->getSourceId());
            }
        }
        if (dir == UL) {
            header->setLteFeedbackDoubleVectorUl(fb);
            // Prepare parameters for next loop iteration - in order to compute SNR in DL
            lteinfo->setTxPower(txPower_);
            lteinfo->setDirection(DL);
            // Get SNR for DL direction
            if (channelModel != nullptr)
                snr = channelModel->getSINR(frame, lteinfo);
            else
                throw cRuntimeError("LtePhyEnbD2D::requestFeedback - channelModel is null pointer. Abort");

            dir = DL;
        }
        else if (dir == DL) {
            header->setLteFeedbackDoubleVectorDl(fb);

            if (enableD2DCqiReporting_) {
                // Compute D2D feedback for all possible peering UEs
                std::vector<UeInfo *> *ueList = binder_->getUeList();
                for (const auto& ueInfo : *ueList) {
                    MacNodeId peerId = ueInfo->id;
                    if (peerId != lteinfo->getSourceId() && binder_->getD2DCapability(lteinfo->getSourceId(), peerId) && binder_->getNextHop(peerId) == nodeId_) {
                        // The source UE might communicate with this peer using D2D, so compute feedback (only in-cell D2D)

                        // Retrieve the position of the peer
                        Coord peerCoord = ueInfo->phy->getCoord();

                        // Get SINR for this link
                        if (channelModel != nullptr)
                            snr = channelModel->getSINR_D2D(frame, lteinfo, peerId, peerCoord, nodeId_);
                        else
                            throw cRuntimeError("LtePhyEnbD2D::requestFeedback - channelModel is null pointer. Abort");

                        // Compute the feedback for this link
                        fb = lteFeedbackComputation_->computeFeedback(type, rbtype, txmode,
                                antennaCws, numPreferredBand, IDEAL, nRus, snr,
                                lteinfo->getSourceId());

                        header->setLteFeedbackDoubleVectorD2D(peerId, fb);
                    }
                }
            }
            dir = UNKNOWN_DIRECTION;
        }
    }
    EV << "LtePhyEn::requestFeedback : Feedback Generated for nodeId: "
       << nodeId_ << " with generator type "
       << fbGeneratorTypeToA(req.genType) << " Fb size: " << fb.size()
       << " Carrier: " << lteinfo->getCarrierFrequency() << endl;

    pktAux->insertAtFront(header);
}

void LtePhyEnbD2D::handleAirFrame(cMessage *msg)
{
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(msg->removeControlInfo());
    LteAirFrame *frame = static_cast<LteAirFrame *>(msg);

    EV << "LtePhyEnbD2D::handleAirFrame - received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    // Handle broadcast packet sent by another eNB
    if (lteInfo->getFrameType() == HANDOVERPKT) {
        EV << "LtePhyEnbD2D::handleAirFrame - received handover packet from another eNodeB. Ignore it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // Check if the air frame was sent on a correct carrier frequency
    double carrierFreq = lteInfo->getCarrierFrequency();
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

    if (lteInfo->getMulticastGroupId() != -1 && !(binder_->isInMulticastGroup(nodeId_, lteInfo->getMulticastGroupId()))) {
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

    int sourceId = binder_->getOmnetId(connectedNodeId_);
    int senderId = binder_->getOmnetId(lteInfo->getDestId());
    if (sourceId == 0 || senderId == 0) {
        // Either source or destination have left the simulation
        delete msg;
        return;
    }

    // Handle all control pkt
    if (handleControlPkt(lteInfo, frame))
        return; // If frame contains a control pkt no further action is needed

    bool result = true;
    RemoteSet r = lteInfo->getUserTxParams()->readAntennaSet();
    if (r.size() > 1) {
        // Use DAS
        // Message from UE
        for (auto it : r) {
            EV << "LtePhy: Receiving Packet from antenna " << it << "\n";

            /*
             * On eNodeB set the current position
             * to the receiving DAS antenna
             */
            // Move set start
            cc->setRadioPosition(myRadioRef, das_->getAntennaCoord(it));

            RemoteUnitPhyData data;
            data.txPower = lteInfo->getTxPower();
            data.m = getRadioPosition();
            frame->addRemoteUnitPhyDataVector(data);
        }
        result = channelModel->isErrorDas(frame, lteInfo);
    }
    else {
        result = channelModel->isError(frame, lteInfo);
    }
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
    lteInfo->setDeciderResult(result);
    auto lteInfoTag = pkt->addTagIfAbsent<UserControlInfo>();
    *lteInfoTag = *lteInfo;
    delete lteInfo;

    // Send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

} //namespace

