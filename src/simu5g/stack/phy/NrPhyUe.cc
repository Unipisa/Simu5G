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

#include "simu5g/stack/phy/NrPhyUe.h"

#include "simu5g/stack/ip2nic/HandoverPacketHolderUe.h"
#include "simu5g/stack/rrc/HandoverController.h"
#include "simu5g/stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "simu5g/stack/rrc/D2dModeSelectionBase.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(NrPhyUe);



void NrPhyUe::initialize(int stage)
{
    LtePhyUeD2D::initialize(stage);
}

// TODO: ***reorganize*** method
void NrPhyUe::handleAirFrame(cMessage *msg)
{
    LteAirFrame *frame = static_cast<LteAirFrame *>(msg);
    UserControlInfo *lteInfo = new UserControlInfo(frame->getAdditionalInfo());

    EV << "NrPhyUe: received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    MacNodeId sourceId = lteInfo->getSourceId();
    if (!binder_->nodeExists(sourceId)) {
        // The source has left the simulation
        delete msg;
        return;
    }

    GHz carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel *channelModel = getChannelModel(carrierFreq);
    if (channelModel == nullptr) {
        EV << "Received packet on carrier frequency not supported by this node. Delete it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    //Update coordinates of this user
    if (lteInfo->getFrameType() == BEACONPKT) {
        // Check if the message is on another carrier frequency
        if (carrierFreq != primaryChannelModel_->getCarrierFrequency()) {
            EV << "Received beacon packet on a different carrier frequency. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        // Check if the message is from a different cellular technology
        if (lteInfo->isNr() != isNr_) {
            EV << "Received beacon packet [from NR=" << lteInfo->isNr() << "] from a different radio technology [to NR=" << isNr_ << "]. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        handoverController_->beaconReceived(frame, lteInfo);
        return;
    }

    // Check if the frame is for us ( MacNodeId matches or - if this is a multicast communication - enrolled in multicast group)
    if (lteInfo->getDestId() != nodeId_ && !(binder_->isInMulticastGroup(nodeId_, lteInfo->getPacketMulticastGroupId()))) {
        EV << "ERROR: Frame is not for us. Delete it." << endl;
        EV << "Packet Type: " << phyFrameTypeToA((LtePhyFrameType)lteInfo->getFrameType()) << endl;
        EV << "Frame MacNodeId: " << lteInfo->getDestId() << endl;
        EV << "Local MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    /*
     * This could happen if the UE associates with a new master while a packet from the
     * old master is in-flight: the packet is in the air
     * while the UE changes master.
     * Event timing:      TTI x: packet scheduled and sent by the UE (tx time = 1ms)
     *                     TTI x+0.1: UE changes master
     *                     TTI x+1: packet from UE arrives at the old master
     */
    if (lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI && lteInfo->getSourceId() != servingNodeId_) {
        EV << "WARNING: Frame from a UE that is leaving this cell (handover): deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "UE MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    if (binder_->isInMulticastGroup(nodeId_, lteInfo->getPacketMulticastGroupId())) {
        // HACK: If this is a multicast connection, change the destId of the airframe so that upper layers can handle it
        lteInfo->setDestId(nodeId_);
    }

    // Send H-ARQ feedback up
    if (lteInfo->getFrameType() == HARQPKT || lteInfo->getFrameType() == GRANTPKT || lteInfo->getFrameType() == RACPKT || lteInfo->getFrameType() == D2DMODESWITCHPKT) {
        handleControlMsg(frame, lteInfo);
        return;
    }

    // This is a DATA packet

    // If the packet is a D2D multicast one, store it and decode it at the end of the TTI
    if (d2dMulticastEnableCaptureEffect_ && binder_->isInMulticastGroup(nodeId_, lteInfo->getPacketMulticastGroupId())) {
        // If not already started, auto-send a message to signal the presence of data to be decoded
        if (d2dDecodingTimer_ == nullptr) {
            d2dDecodingTimer_ = new cMessage("d2dDecodingTimer");
            d2dDecodingTimer_->setSchedulingPriority(10);          // Last thing to be performed in this TTI
            scheduleAt(NOW, d2dDecodingTimer_);
        }

        // Store frame, together with related control info
        frame->setControlInfo(lteInfo);
        storeAirFrame(frame);            // Implements the capture effect

        return;                          // Exit the function, decoding will be done later
    }

    if ((lteInfo->getUserTxParams()) != nullptr) {
        int cw = lteInfo->getCw();
        if (lteInfo->getUserTxParams()->readCqiVector().size() == 1)
            cw = 0;
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];
        if (lteInfo->getDirection() == DL) {
            emit(averageCqiDlSignal_, cqi);
            recordCqi(cqi, DL);
        }
    }

    bool result = channelModel->isReceptionSuccessful(frame, lteInfo);

    // Update statistics
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << (result ? "RECEIVED" : "NOT RECEIVED") << endl;

    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());

    // Here frame has to be destroyed since it is no more useful
    delete frame;

    // Attach the decider result to the packet as control info
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
    delete lteInfo;

    pkt->addTagIfAbsent<PhyReceptionInd>()->setDeciderResult(result);

    // Send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

} //namespace
