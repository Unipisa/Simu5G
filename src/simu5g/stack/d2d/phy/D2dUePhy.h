//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef STACK_D2D_PHY_D2DUEPHY_H_
#define STACK_D2D_PHY_D2DUEPHY_H_

#include "simu5g/stack/phy/PhyUe.h"
#include "simu5g/stack/phy/packet/LteFeedbackPkt.h"
#include "simu5g/stack/rrc/ConnectionControlUe.h"
#include "simu5g/stack/d2d/phy/D2dUePhyHelper.h"
#include "simu5g/stack/d2d/binder/D2dBinder.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

using namespace omnetpp;

/**
 * CRTP mixin that adds D2D support to a UE PHY.
 *
 * The core UE PHY (PhyUe) carries no D2D code; this mixin layers the D2D
 * UE-PHY logic (D2D Tx power, D2D CQI accounting, one-to-many D2D
 * transmission, the D2D-multicast capture effect) on top of it. The
 * capture-effect machinery lives in D2dUePhyHelper; the mixin holds the
 * module-side glue. It has native protected access to the Base internals it
 * needs.
 *
 * The concrete Define_Module'd PHY is PhyUeD2D = D2dUePhy<PhyUe>.
 *
 * Note: handleAirFrame() fully replaces the Base implementation; the other
 * overrides extend Base behavior and chain up to it.
 */
template<class Base>
class D2dUePhy : public Base
{
  protected:
    // holds the D2D-specific UE-PHY state and logic:
    // D2D Tx power and the D2D-multicast capture-effect machinery
    D2dUePhyHelper d2dHelper_{this};

    // timer for triggering decoding at the end of the TTI. Started when the first
    // airframe is received. The self-message stays in the module (which owns it);
    // the captured frames it decodes live in d2dHelper_.
    cMessage *d2dDecodingTimer_ = nullptr;

    // Interned in initialize() rather than registered by a static initializer, so that
    // linking the D2D package in cannot shift the signal ids the core assigns. See the
    // "Signals" note in D2dUeMacBase.h.
    simsignal_t averageCqiD2DSignal_ = SIMSIGNAL_NULL;

    // the global D2D directory; this PHY registers itself in it so that peers can
    // resolve it by node id on the one-to-many transmit path
    inet::ModuleRefByPar<D2dBinder> d2dBinder_;

    void initialize(int stage) override;
    void handleSelfMessage(cMessage *msg) override;

    // ---- incoming-frame seams (replace the historical handleAirFrame copy) ----

    /// D2D/D2D_MULTI frames legitimately arrive from peers, not the serving cell
    bool isStaleFrame(const UserControlInfo *lteInfo) override
    {
        return lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI
               && lteInfo->getSourceId() != this->servingNodeId_;
    }

    /// HACK: if this is a multicast connection, change the destId of the
    /// airframe so that upper layers can handle it
    void frameAccepted(UserControlInfo *lteInfo) override
    {
        if (this->binder_->isInMulticastGroup(this->nodeId_, lteInfo->getPacketMulticastGroupId()))
            lteInfo->setDestId(this->nodeId_);
    }

    /// D2D mode-switch notifications are control frames too
    bool isControlFrameType(LtePhyFrameType type) override
    {
        return type == D2DMODESWITCHPKT || Base::isControlFrameType(type);
    }

    /// D2D-multicast capture effect: store the frame and decode it at the end of the TTI
    bool interceptIncomingFrame(LteAirFrame *frame, UserControlInfo *lteInfo) override
    {
        if (!(d2dHelper_.getMulticastEnableCaptureEffect() && this->binder_->isInMulticastGroup(this->nodeId_, lteInfo->getPacketMulticastGroupId())))
            return false;

        // If not already started, auto-send a message to signal the presence of data to be decoded
        if (d2dDecodingTimer_ == nullptr) {
            d2dDecodingTimer_ = new cMessage("d2dDecodingTimer");
            d2dDecodingTimer_->setSchedulingPriority(10);          // last thing to be performed in this TTI
            this->scheduleAt(NOW, d2dDecodingTimer_);
        }

        // Store frame, together with related control info
        frame->setControlInfo(lteInfo);
        d2dHelper_.storeAirFrame(frame);            // implements the capture effect
        return true;
    }

    // ---- outgoing-frame seams (replace the historical handleUpperMessage copy) ----

    /// the D2D UE PHY performs no serving-cell check on outgoing frames
    /// (D2D/D2D_MULTI frames legitimately target peers)
    void validateOutgoingFrame(const UserControlInfo *info) override {}

    /// D2D CQI accounting for outgoing data packets
    void recordExtraTxCqi(double cqi, const UserControlInfo *info) override
    {
        if (info->getDirection() == D2D || info->getDirection() == D2D_MULTI)
            this->emit(averageCqiD2DSignal_, cqi);
    }

    /// keep the historical D2D frame naming (all control frames named "harqFeedback-grant")
    const char *airFrameNameFor(const UserControlInfo *info) override
    {
        switch (info->getFrameType()) {
            case HARQPKT:
            case GRANTPKT:
            case RACPKT: return "harqFeedback-grant";
            default: return "airframe";
        }
    }

    void stampExtraTxControlInfo(UserControlInfo *info) override
    {
        info->setD2dTxPower(d2dHelper_.getD2dTxPower());
    }

    /// one-to-many D2D transmissions go out via sendDirect to all group members
    void transmitFrame(LteAirFrame *frame, const UserControlInfo *info) override
    {
        if (info->getDirection() == D2D_MULTI)
            sendMulticast(frame);
        else
            this->sendUnicast(frame);
    }

    /**
     * Sends a frame to the UEs registered to the multicast group indicated in
     * the frame (optionally skipping receivers beyond multicastD2DRange).
     * Frames are sent with zero transmission delay. D2D-specific: only the
     * D2D UE PHY originates one-to-many D2D transmissions.
     */
    void sendMulticast(LteAirFrame *frame);

  public:
    double getTxPwr(Direction dir = UNKNOWN_DIRECTION) override
    {
        if (dir == D2D)
            return d2dHelper_.getD2dTxPower();
        return this->txPower_;
    }
};

template<class Base>
void D2dUePhy<Base>::initialize(int stage)
{
    Base::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        averageCqiD2DSignal_ = cComponent::registerSignal("averageCqiD2D");
        d2dHelper_.setD2dTxPower(this->par("d2dTxPower"));
        d2dHelper_.setMulticastEnableCaptureEffect(this->par("d2dMulticastCaptureEffect"));
        d2dHelper_.setMulticastD2DRangeCheckEnabled(this->par("enableMulticastD2DRangeCheck"));
        d2dHelper_.setMulticastD2DRange(this->par("multicastD2DRange"));
        d2dBinder_.reference(this, "d2dBinderModule", true);
        d2dHelper_.initWatches();
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        // Publish this PHY under its node id, so that a peer's one-to-many transmit
        // path can reach it without knowing the NIC's submodule names
        d2dBinder_->registerD2dPhy(this->nodeId_, this);
    }
}

template<class Base>
void D2dUePhy<Base>::handleSelfMessage(cMessage *msg)
{
    if (msg->isName("d2dDecodingTimer")) {
        // Decode the captured frame (capture effect) and clear the receive buffer.
        d2dHelper_.decodeStoredFrames();

        delete msg;
        d2dDecodingTimer_ = nullptr;
    }
    else
        Base::handleSelfMessage(msg);
}

template<class Base>
void D2dUePhy<Base>::sendMulticast(LteAirFrame *frame)
{
    UserControlInfo *ci = check_and_cast<UserControlInfo *>(frame->getControlInfo());

    // get the group Id
    MacNodeId groupId = ci->getPacketMulticastGroupId();
    if (groupId == NODEID_NONE)
        throw cRuntimeError("D2dUePhy::sendMulticast - Error. Group ID %d is not valid.", num(groupId));

    // transfer control info into airframe fields
    frame->setAdditionalInfo(*ci);
    delete frame->removeControlInfo();

    // send the frame to nodes belonging to the multicast group only
    for (auto [destId, nodeInfo] : this->binder_->getNodeInfoMap()) {
        // if the node in the list does not use the same LTE/NR technology of this PHY module, skip it
        if (isNrUe(destId) != this->isNr_)
            continue;

        if (destId != this->nodeId_ && this->binder_->isInMulticastGroup(destId, groupId)) {
            EV << NOW << " D2dUePhy::sendMulticast - node " << destId << " is in the multicast group" << endl;

            // get a pointer to receiving module
            cModule *receiver = nodeInfo.moduleRef;

            if (d2dHelper_.getMulticastD2DRangeCheckEnabled()) {
                // the peer's PHY registered itself under its node id at init; the
                // technology already matches, since the loop skipped the other leg above
                PhyBase *recvPhy = d2dBinder_->getD2dPhy(destId);
                if (recvPhy == nullptr)
                    throw cRuntimeError("D2dUePhy::sendMulticast - node %d is in multicast group %d but has no registered D2D PHY",
                            num(destId), num(groupId));

                double dist = recvPhy->getCoord().distance(this->getRadioPosition());

                if (dist > d2dHelper_.getMulticastD2DRange()) {
                    EV << NOW << " D2dUePhy::sendMulticast - node too far (" << dist << " > " << d2dHelper_.getMulticastD2DRange() << ". skipping transmission" << endl;
                    continue;
                }
            }

            EV << NOW << " D2dUePhy::sendMulticast - sending frame to node " << destId << endl;

            // Create a duplicate frame before sending
            LteAirFrame *frameToSend = frame->dup();
            this->sendDirect(frameToSend, 0, frame->getDuration(), receiver, this->getReceiverGateIndex(receiver, destId));
        }
    }

    // delete the original frame
    delete frame;
}

} //namespace

#endif
