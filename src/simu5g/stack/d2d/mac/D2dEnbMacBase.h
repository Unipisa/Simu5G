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

#ifndef STACK_D2D_MAC_D2DENBMACBASE_H_
#define STACK_D2D_MAC_D2DENBMACBASE_H_

#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "simu5g/stack/d2d/mac/harq/LteHarqBufferMirrorD2D.h"
#include "simu5g/stack/d2d/mac/harq/LteHarqBufferRxD2D.h"
#include "simu5g/stack/d2d/mac/amc/AmcPilotD2D.h"
#include "simu5g/stack/d2d/mac/conflictgraph/ConflictGraph.h"
#include "simu5g/stack/d2d/mac/ID2dAmc.h"
#include "simu5g/stack/d2d/mac/ID2dMacEnb.h"
#include "simu5g/stack/d2d/mac/D2dEnbMacHelper.h"
#include "simu5g/stack/d2d/rrc/D2DModeSwitchNotification_m.h"
#include "simu5g/stack/phy/packet/LteFeedbackPkt.h"
#include "simu5g/stack/rrc/ConnectionControlEnb.h"

namespace simu5g {

using namespace omnetpp;

/**
 * CRTP mixin that adds D2D support to an eNB/gNB MAC.
 *
 * The core MACs (LteMacEnb and its NR subclass NrMacGnb) carry no D2D code;
 * this mixin layers the D2D eNB-MAC glue (D2D feedback push, mode-switch
 * dispatch and notification, mirror H-ARQ buffer lifecycle, conflict-graph
 * bookkeeping) on top of either of them, so the LTE and NR D2D MACs share one
 * implementation. The heavy shared state and logic already lives in
 * D2dEnbMacHelper; the mixin holds the module-side glue that the two leaf
 * classes used to duplicate. It has native protected access to the Base
 * internals it needs.
 *
 * The concrete Define_Module'd MACs are:
 *   LteMacEnbD2D = D2dEnbMacBase<LteMacEnb> (+ the sendGrants/macPduUnmake
 *                  fork overrides, see LteMacEnbD2D.h)
 *   NrMacGnbD2D  = D2dEnbMacBase<NrMacGnb>
 *
 * No signals are owned here: the mode-switch statistics are declared on the D2D
 * NED modules and emitted by the UE-side mixin. See the "Signals" note in
 * D2dUeMacBase.h for the package-wide registration rule.
 */
template<class Base>
class D2dEnbMacBase : public Base, public ID2dMacEnb
{
  protected:
    // holds the D2D-specific eNB-MAC state and logic
    D2dEnbMacHelper d2dEnbHelper_;

    void macHandleFeedbackPkt(cPacket *pkt) override;

    /**
     * Flush Tx H-ARQ buffers for all users; additionally marks the D2D
     * mirror buffers as waiting.
     */
    void flushHarqBuffers() override;

    /// Lower Layer Handler: intercepts mirrored D2D H-ARQ feedback
    void fromPhy(cPacket *pkt) override;

    /// HARQ RX buffer factory: adds support for the D2D and D2D_MULTI directions
    LteHarqBufferRx *createRxHarqBuffer(MacNodeId src, const UserControlInfo *userInfo) override;

    // the node's control-plane entry point, which knows the UEs handing over
    inet::ModuleRefByPar<ConnectionControlEnb> connectionControl_;

  public:
    D2dEnbMacBase() : d2dEnbHelper_(this)
    {
    }

    /**
     * Reads the D2D MAC parameters and performs initialization.
     */
    void initialize(int stage) override;

    /// intercepts the D2D self-messages (mode switch, conflict graph update)
    void handleMessage(cMessage *msg) override;

    // ---- ID2dMacEnb ----

    bool isReuseD2DEnabled() override
    {
        return d2dEnbHelper_.getReuseD2D();
    }

    bool isReuseD2DMultiEnabled() override
    {
        return d2dEnbHelper_.getReuseD2DMulti();
    }

    ConflictGraph *getConflictGraph() override
    {
        return d2dEnbHelper_.getConflictGraph();
    }

    void deleteHarqBuffersMirrorD2D(MacNodeId txPeer, MacNodeId rxPeer) override;

    /**
     * deleteQueues() on ENB performs actions
     * from base classes and also deletes mirror buffers
     *
     * @param nodeId id of node performing handover
     */
    void deleteQueues(MacNodeId nodeId) override;

    // get the reference to the "mirror" buffers
    HarqBuffersMirrorD2D *getHarqBuffersMirrorD2D(GHz carrierFrequency) override;

    // delete the "mirror" Harq Buffer for this node (useful at handover)
    virtual void deleteHarqBuffersMirrorD2D(MacNodeId nodeId);

    // send the D2D Mode Switch signal to the transmitter of the given flow
    void sendModeSwitchNotification(MacNodeId srcId, MacNodeId dst, LteD2DMode oldMode, LteD2DMode newMode) override;

    bool isMsHarqInterrupt() override { return d2dEnbHelper_.getMsHarqInterrupt(); }
};

template<class Base>
void D2dEnbMacBase<Base>::initialize(int stage)
{
    Base::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
        connectionControl_.reference(this, "connectionControlModule", true);
    // (the AMC pilot/mode-switch parameter setup historically also ran at
    // INITSTAGE_PHYSICAL_ENVIRONMENT -- an identical, idempotent copy of the
    // INITSTAGE_SIMU5G_AMC_SETUP block below; the early copy is gone)
    if (stage == inet::INITSTAGE_LAST) { // be sure that all UEs have been initialized
        d2dEnbHelper_.setReuseD2D(this->par("reuseD2D"));
        d2dEnbHelper_.setReuseD2DMulti(this->par("reuseD2DMulti"));

        if (d2dEnbHelper_.getReuseD2D() || d2dEnbHelper_.getReuseD2DMulti()) {
            d2dEnbHelper_.setConflictGraphUpdatePeriod(this->par("conflictGraphUpdatePeriod"));

            d2dEnbHelper_.createDistanceBasedConflictGraph(this->binder_, this->par("conflictGraphThreshold"),
                    this->par("conflictGraphD2DInterferenceRadius"), this->par("conflictGraphD2DMultiTxRadius"), this->par("conflictGraphD2DMultiInterferenceRadius"));

            this->scheduleAt(NOW + 0.05, new cMessage("updateConflictGraph"));
        }
    }
    else if (stage == INITSTAGE_SIMU5G_AMC_SETUP) {
        bool usePreconfiguredTxParams = this->par("usePreconfiguredTxParams");
        Cqi d2dCqi = this->par("d2dCqi");
        if (usePreconfiguredTxParams)
            check_and_cast<AmcPilotD2D *>(this->amc_->getPilot())->setPreconfiguredTxParams(d2dCqi);

        d2dEnbHelper_.setMsHarqInterrupt(this->par("msHarqInterrupt").boolValue());
        d2dEnbHelper_.setMsClearRlcBuffer(this->par("msClearRlcBuffer").boolValue());
    }
}

template<class Base>
void D2dEnbMacBase<Base>::macHandleFeedbackPkt(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto fb = pkt->peekAtFront<LteFeedbackPkt>();
    auto lteInfo = pkt->getTag<UserControlInfo>();

    std::map<MacNodeId, LteFeedbackDoubleVector> fbMapD2D = fb->getLteFeedbackDoubleVectorD2D();

    // skip if no D2D CQI has been reported
    if (!fbMapD2D.empty()) {
        // the AMC of a D2D-capable eNB is always a D2D AMC
        ID2dAmc *d2dAmc = check_and_cast<ID2dAmc *>(this->amc_);

        MacNodeId id = fb->getSourceNodeId();

        // extract feedback for D2D links
        for (const auto& mapIt : fbMapD2D) {
            MacNodeId peerId = mapIt.first;
            for (const auto& it : mapIt.second) {
                for (const auto& jt : it) {
                    if (!jt.isEmptyFeedback()) {
                        d2dAmc->pushFeedbackD2D(id, jt, peerId, lteInfo->getCarrierFrequency());
                    }
                }
            }
        }
    }
    Base::macHandleFeedbackPkt(pkt);
}

template<class Base>
void D2dEnbMacBase<Base>::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage() && msg->isName("D2DModeSwitchNotification")) {
        cPacket *pkt = check_and_cast<cPacket *>(msg);
        d2dEnbHelper_.macHandleD2DModeSwitch(pkt);
        delete pkt;
    }
    else if (msg->isSelfMessage() && msg->isName("updateConflictGraph")) {
        // compute conflict graph for resource allocation
        d2dEnbHelper_.computeConflictGraph();

        this->scheduleAt(NOW + d2dEnbHelper_.getConflictGraphUpdatePeriod(), msg);
    }
    else
        Base::handleMessage(msg);
}

template<class Base>
HarqBuffersMirrorD2D *D2dEnbMacBase<Base>::getHarqBuffersMirrorD2D(GHz carrierFrequency)
{
    return d2dEnbHelper_.getHarqBuffersMirrorD2D(carrierFrequency);
}

template<class Base>
void D2dEnbMacBase<Base>::deleteQueues(MacNodeId nodeId)
{
    Base::deleteQueues(nodeId);
    deleteHarqBuffersMirrorD2D(nodeId);
}

template<class Base>
void D2dEnbMacBase<Base>::deleteHarqBuffersMirrorD2D(MacNodeId nodeId)
{
    d2dEnbHelper_.deleteHarqBuffersMirrorD2D(nodeId);
}

template<class Base>
void D2dEnbMacBase<Base>::deleteHarqBuffersMirrorD2D(MacNodeId txPeer, MacNodeId rxPeer)
{
    d2dEnbHelper_.deleteHarqBuffersMirrorD2D(txPeer, rxPeer);
}

template<class Base>
void D2dEnbMacBase<Base>::sendModeSwitchNotification(MacNodeId srcId, MacNodeId dstId, LteD2DMode oldMode, LteD2DMode newMode)
{
    Enter_Method_Silent("sendModeSwitchNotification");

    EV << NOW << " D2dEnbMacBase::sendModeSwitchNotification - " << srcId << " --> " << dstId << " going from " << d2dModeToA(oldMode) << " to " << d2dModeToA(newMode) << endl;

    // send switch notification to both the tx and rx side of the flow

    auto pktTx = new inet::Packet("D2DModeSwitchNotification");

    auto switchPktTx = inet::makeShared<D2DModeSwitchNotification>();
    switchPktTx->setTxSide(true);
    switchPktTx->setPeerId(dstId);
    switchPktTx->setOldMode(oldMode);
    switchPktTx->setNewMode(newMode);
    switchPktTx->setInterruptHarq(d2dEnbHelper_.getMsHarqInterrupt());
    switchPktTx->setClearRlcBuffer(d2dEnbHelper_.getMsClearRlcBuffer());

    pktTx->addTagIfAbsent<UserControlInfo>()->setSourceId(this->nodeId_);
    pktTx->addTagIfAbsent<UserControlInfo>()->setDestId(srcId);
    pktTx->addTagIfAbsent<UserControlInfo>()->setFrameType(D2DMODESWITCHPKT);

    pktTx->insertAtFront(switchPktTx);
    auto switchPktTx_local = pktTx->dup();
    this->sendLowerPackets(pktTx);

    auto pktRx = new inet::Packet("D2DModeSwitchNotification");
    auto switchPktRx = inet::makeShared<D2DModeSwitchNotification>();
    switchPktRx->setTxSide(false);
    switchPktRx->setPeerId(srcId);
    switchPktRx->setOldMode(oldMode);
    switchPktRx->setNewMode(newMode);
    switchPktRx->setInterruptHarq(d2dEnbHelper_.getMsHarqInterrupt());
    switchPktRx->setClearRlcBuffer(d2dEnbHelper_.getMsClearRlcBuffer());

    pktRx->addTagIfAbsent<UserControlInfo>()->setSourceId(this->nodeId_);
    pktRx->addTagIfAbsent<UserControlInfo>()->setDestId(dstId);
    pktRx->addTagIfAbsent<UserControlInfo>()->setFrameType(D2DMODESWITCHPKT);
    pktRx->insertAtFront(switchPktRx);

    auto switchPktRx_local = pktRx->dup();
    this->sendLowerPackets(pktRx);

    this->scheduleAt(NOW + TTI, switchPktTx_local);
    this->scheduleAt(NOW + TTI, switchPktRx_local);
}

template<class Base>
void D2dEnbMacBase<Base>::flushHarqBuffers()
{
    Base::flushHarqBuffers();

    // flush mirror buffer
    for (auto& mirr_mit : d2dEnbHelper_.getHarqBuffersMirrorD2DMap()) {
        for (auto& mirr_it : mirr_mit.second)
            mirr_it.second->markSelectedAsWaiting();
    }
}

template<class Base>
LteHarqBufferRx *D2dEnbMacBase<Base>::createRxHarqBuffer(MacNodeId src, const UserControlInfo *userInfo)
{
    Direction dir = (Direction)userInfo->getDirection();
    if (dir == D2D || dir == D2D_MULTI)
        return new LteHarqBufferRxD2D(this->harqProcesses_, this, this->binder_, src, (dir == D2D_MULTI));
    return Base::createRxHarqBuffer(src, userInfo);
}

template<class Base>
void D2dEnbMacBase<Base>::fromPhy(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto userInfo = pkt->getTag<UserControlInfo>();
    if (userInfo->getFrameType() == HARQPKT) {
        MacNodeId src = userInfo->getSourceId();
        GHz carrierFrequency = userInfo->getCarrierFrequency();

        // this feedback refers to a mirrored H-ARQ buffer
        auto hfbpkt = pkt->peekAtFront<LteHarqFeedback>();
        if (!hfbpkt->getD2dFeedback()) { // this is not a mirror feedback
            LteMacBase::fromPhy(pkt);
            return;
        }

        // H-ARQ feedback, send it to the mirror buffer of the D2D pair
        auto mfbpkt = pkt->peekAtFront<LteHarqFeedbackMirror>();
        MacNodeId d2dSender = mfbpkt->getD2dSenderId();
        MacNodeId d2dReceiver = mfbpkt->getD2dReceiverId();
        D2DPair pair(d2dSender, d2dReceiver);
        auto& harqBuffersMirrorD2D = d2dEnbHelper_.getHarqBuffersMirrorD2DMap();
        HarqBuffersMirrorD2D::iterator hit = harqBuffersMirrorD2D[carrierFrequency].find(pair);
        EV << NOW << "D2dEnbMacBase::fromPhy - node " << this->nodeId_ << " Received HARQ Feedback pkt (mirrored)" << endl;
        if (hit == harqBuffersMirrorD2D[carrierFrequency].end()) {
            // if feedback arrives, a buffer should exist (unless it is a handover scenario
            // where the HARQ buffer was deleted but feedback was in transit)
            // this case must be taken care of
            if (connectionControl_->isHandingOver(src))
                return;

            // create buffer
            LteHarqBufferMirrorD2D *hb = new LteHarqBufferMirrorD2D((unsigned int)this->harqProcesses_, (unsigned char)this->par("maxHarqRtx"), this);
            harqBuffersMirrorD2D[carrierFrequency][pair] = hb;
            hb->receiveHarqFeedback(pkt);
        }
        else {
            hit->second->receiveHarqFeedback(pkt);
        }
    }
    else {
        LteMacBase::fromPhy(pkt);
    }
}

} //namespace

#endif
