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

#ifndef STACK_D2D_MAC_D2DUEMACBASE_H_
#define STACK_D2D_MAC_D2DUEMACBASE_H_

#include "simu5g/stack/mac/LteMacUe.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/mac/amc/LteAmc.h"
#include "simu5g/stack/mac/buffer/LteMacBuffer.h"
#include "simu5g/stack/mac/buffer/LteMacQueue.h"
#include "simu5g/stack/mac/packet/LteMacPdu.h"
#include "simu5g/stack/mac/packet/LteRac_m.h"
#include "simu5g/stack/mac/packet/LteSchedulingGrant.h"
#include "simu5g/stack/phy/channelmodel/ChannelModelBase.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/stack/d2d/mac/ID2dMacEnb.h"
#include "simu5g/stack/d2d/mac/ID2dMacUe.h"
#include "simu5g/stack/d2d/mac/D2dUeMacHelper.h"
#include "simu5g/stack/d2d/mac/harq/LteHarqBufferRxD2D.h"
#include "simu5g/stack/d2d/mac/harq/LteHarqBufferTxD2D.h"
#include "simu5g/stack/d2d/mac/scheduler/LcgSchedulerD2D.h"

namespace simu5g {

using namespace omnetpp;

/**
 * CRTP mixin that adds D2D support to a UE MAC.
 *
 * The core UE MACs (LteMacUe and its NR subclass NrMacUe) carry no D2D code;
 * this mixin layers the D2D UE-MAC logic (D2D BSR handling, D2D RAC requests,
 * mode-switch dispatch, D2D H-ARQ buffer factories, the D2D macPduMake seams) on
 * top of either of them, so the LTE and NR D2D MACs share one implementation.
 * The D2D state and mode-switch machinery live in D2dUeMacHelper; the mixin
 * holds the module-side logic the two leaf classes used to duplicate. It has
 * native protected access to the Base internals it needs.
 *
 * The concrete Define_Module'd MACs are:
 *   LteMacUeD2D = D2dUeMacBase<LteMacUe> (+ the LTE-specific main loop, see
 *                 LteMacUeD2D.h)
 *   NrMacUeD2D  = D2dUeMacBase<NrMacUe>
 *
 * The LTE/NR differences inside the shared macPduMake body flow through the
 * two D2D-agnostic Base seams isCarrierActive() (numerology period) and
 * reserveTxHarqUnits() (synchronous vs. asynchronous H-ARQ), preserving each
 * variant's historical behavior exactly.
 *
 * Signals: every D2D signal id in this package is obtained by interning the name
 * at RUNTIME -- from a module's initialize(), or from a helper's constructor --
 * never by a static registerSignal() initializer. Static registration happens at
 * load time in link order, so a static in any D2D translation unit would insert
 * D2D names into the middle of the core's signal-id sequence, shifting the order
 * in which results are recorded and with it the 'sz' fingerprint ingredient, in a
 * way that depends on the link line. Interning at runtime keeps every core signal
 * id identical whether or not the D2D package is linked in, which is what makes
 * the feature-off build genuinely equivalent to the core alone.
 */
template<class Base>
class D2dUeMacBase : public Base, public ID2dMacUe
{
  protected:
    // holds the D2D-specific UE-MAC state and logic
    D2dUeMacHelper d2dUeHelper_{this};

    // buffer-overflow statistic for the D2D directions; the core MAC owns only the
    // DL/UL ones. Interned in initialize() -- see the "Signals" note above.
    simsignal_t macBufferOverflowD2DSignal_ = SIMSIGNAL_NULL;

    /// The D2D directions are serviced here; everything else defers to the core MAC.
    void recordBufferOverflow(Direction dir, double sample) override
    {
        if (dir == D2D || dir == D2D_MULTI)
            this->emit(macBufferOverflowD2DSignal_, sample);
        else
            Base::recordBufferOverflow(dir, sample);
    }

    /**
     * Reads MAC parameters for the UE and performs initialization.
     */
    void initialize(int stage) override;

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    void handleMessage(cMessage *msg) override;

    /// D2D delta for the base main loop: a pending D2D multicast BSR also
    /// warrants a BSR-only MAC PDU.
    bool isBsrPending() const override { return this->bsrTriggered_ || d2dUeHelper_.getBsrD2DMulticastTriggered(); }

    /// Extends the base by also clearing the D2D-multicast trigger, so that one
    /// appended BSR consumes whichever of the two triggers was pending.
    void appendBsr(inet::Ptr<LteMacPdu> macPdu) override
    {
        Base::appendBsr(macPdu);
        d2dUeHelper_.setBsrD2DMulticastTriggered(false);
    }

    /// A UE in D2D mode that is granted UL resources with nothing scheduled answers
    /// with a standalone BSR-only PDU covering just its D2D (or D2D multicast) flows.
    /// Returns true if such a PDU was built and placed in macPduList_, in which case
    /// macPduMake() skips the SDU loop entirely.
    bool buildStandaloneBsr() override;

    /// D2D bearers are peer-addressed: the destination comes from the flow, not the cell.
    MacNodeId pduDestId(MacCid destCid) override { return this->connDescOut_.at(destCid).flowInfo.getDestId(); }

    /// An entry with no SDUs is only worth a PDU if it can carry a pending BSR
    /// (either the Uu one or the D2D multicast one -- see isBsrPending()).
    bool shouldSkipScheduleEntry(unsigned int sduPerCid) const override { return sduPerCid == 0 && !this->isBsrPending(); }

    inet::Packet *createUlMacPdu(MacCid destCid, GHz carrierFreq, MacNodeId destId) override;

    /// D2D_MULTI has no feedback, so it always uses the current process, as the
    /// historical LTE D2D MAC did for every direction; other directions follow the
    /// Base policy (synchronous for LTE, first available for NR).
    UnitList reserveTxHarqUnits(LteHarqBufferTx *txBuf, Direction dir) override
    {
        return dir == D2D_MULTI ? txBuf->getEmptyUnits(this->currentHarq_) : Base::reserveTxHarqUnits(txBuf, dir);
    }

    void macHandleGrant(cPacket *pkt) override;

    /*
     * Checks RAC status
     */
    void checkRAC() override;

    /*
     * Receives and handles RAC responses
     */
    void macHandleRac(cPacket *pkt) override;

    virtual void macHandleD2DModeSwitch(cPacket *pkt);

    /// HARQ buffer factories: add support for the D2D and D2D_MULTI directions
    LteHarqBufferRx *createRxHarqBuffer(MacNodeId src, const UserControlInfo *userInfo) override;
    LteHarqBufferTx *createTxHarqBuffer(MacNodeId destId, Direction dir) override;

    /// Factory override: use the D2D-capable LCG scheduler
    LcgScheduler *createLcgScheduler() override;

  public:
    void doHandover(MacNodeId targetEnb) override;
};

template<class Base>
void D2dUeMacBase<Base>::initialize(int stage)
{
    Base::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
        macBufferOverflowD2DSignal_ = cComponent::registerSignal("macBufferOverFlowD2D");

    if (stage == INITSTAGE_SIMU5G_AMC_ATTACHUSER) {
        // get parameters
        d2dUeHelper_.setUsePreconfiguredTxParams(this->par("usePreconfiguredTxParams"));

        if (this->cellId_ != NODEID_NONE) {
            d2dUeHelper_.rebuildPreconfiguredTxParams(this->binder_);

            // get the reference to the eNB
            d2dUeHelper_.setEnb(check_and_cast<ID2dMacEnb *>(this->binder_->getMacByNodeId(this->cellId_)));

            LteAmc *amc = check_and_cast<LteMacEnb *>(this->binder_->getMacByNodeId(this->cellId_))->getAmc();
            amc->attachUser(this->nodeId_, D2D);
        }
    }
}

template<class Base>
bool D2dUeMacBase<Base>::buildStandaloneBsr()
{
    // UE is in D2D mode but it received an UL grant (for BSR)
    for (auto& [carrierFreq, grant] : this->schedulingGrant_) {
        // skip if this is not the turn of this carrier
        if (!this->isCarrierActive(carrierFreq))
            continue;

        if (grant != nullptr && grant->getDirection() == UL && this->emptyScheduleList_) {
            if (this->bsrTriggered_ || d2dUeHelper_.getBsrD2DMulticastTriggered()) {
                // Compute BSR size taking into account only DM flows
                int sizeBsr = 0;
                for (auto [cid, connInfo] : this->connDescOut_) {
                    Direction connDir = connInfo.flowInfo.getDirection();

                    // if the bsr was triggered by D2D (D2D_MULTI), only account for D2D (D2D_MULTI) connections
                    if (this->bsrTriggered_ && connDir != D2D)
                        continue;
                    if (d2dUeHelper_.getBsrD2DMulticastTriggered() && connDir != D2D_MULTI)
                        continue;

                    sizeBsr += connInfo.buffer->getQueueOccupancy();

                    // take into account the RLC header size
                    if (sizeBsr > 0) {
                        if (this->getLogicalChannelConfig(cid).rlcMode == UM)
                            sizeBsr += RLC_HEADER_UM;
                        else if (this->getLogicalChannelConfig(cid).rlcMode == AM)
                            sizeBsr += RLC_HEADER_AM;
                    }
                }

                if (sizeBsr > 0) {
                    // Call the appropriate function to make a BSR for a D2D communication
                    LogicalCid bsrType = d2dUeHelper_.getBsrD2DMulticastTriggered() ? D2D_MULTI_SHORT_BSR : D2D_SHORT_BSR;
                    d2dUeHelper_.setBsrD2DMulticastTriggered(false);
                    inet::Packet *macPktBsr = d2dUeHelper_.makeBsr(sizeBsr);
                    auto info = macPktBsr->getTagForUpdate<UserControlInfo>();
                    info->setPacketLcid(bsrType);
                    info->setCarrierFrequency(carrierFreq);
                    info->setUserTxParams(grant->getUserTxParams()->dup());

                    // Add the created BSR to the PDU List
                    // select channel model for the given carrier frequency
                    ChannelModelBase *channelModel = this->phy_->getChannelModel(carrierFreq);
                    if (channelModel == nullptr)
                        throw cRuntimeError("D2dUeMacBase::buildStandaloneBsr - channel model is a null pointer");
                    this->macPduList_[channelModel->getCarrierFrequency()][{this->getMacCellId(), 0}] = macPktBsr;
                    EV << "D2dUeMacBase::buildStandaloneBsr - BSR D2D created with size " << sizeBsr << " bytes created" << endl;

                    this->bsrRtxTimer_ = this->bsrRtxTimerStart_;  // this prevents the UE from sending an unnecessary RAC request
                    return true;
                }
                else {
                    d2dUeHelper_.setBsrD2DMulticastTriggered(false);
                    this->bsrTriggered_ = false;
                    this->bsrRtxTimer_ = 0;
                }
            }
            // the first carrier whose grant matched decides; the historical loop
            // broke out here whether or not a BSR was actually built
            return false;
        }
    }
    return false;
}

template<class Base>
Packet *D2dUeMacBase<Base>::createUlMacPdu(MacCid destCid, GHz carrierFreq, MacNodeId destId)
{
    auto macPkt = new inet::Packet("LteMacPdu");
    auto header = inet::makeShared<LteMacPdu>();
    header->setHeaderLength(MAC_HEADER);
    macPkt->insertAtFront(header);

    // the direction is the flow's (UL, D2D or D2D_MULTI), not a constant
    auto info = macPkt->template addTagIfAbsent<UserControlInfo>();
    info->setSourceId(this->getMacNodeId());
    info->setDestId(destId);
    info->setDirection(this->connDescOut_.at(destCid).flowInfo.getDirection());
    info->setPacketLcid(SHORT_BSR);
    info->setCarrierFrequency(carrierFreq);
    info->setGrantId(this->schedulingGrant_[carrierFreq]->getGrantId());

    // a D2D transmitter may be configured to ignore the grant's parameters
    if (d2dUeHelper_.getUsePreconfiguredTxParams())
        info->setUserTxParams(d2dUeHelper_.getPreconfiguredTxParams()->dup());
    else
        info->setUserTxParams(this->schedulingGrant_[carrierFreq]->getUserTxParams()->dup());

    return macPkt;
}

template<class Base>
LteHarqBufferRx *D2dUeMacBase<Base>::createRxHarqBuffer(MacNodeId src, const UserControlInfo *userInfo)
{
    Direction dir = (Direction)userInfo->getDirection();
    if (dir == D2D || dir == D2D_MULTI)
        return new LteHarqBufferRxD2D(this->harqProcesses_, this, this->binder_, src, (dir == D2D_MULTI));
    return Base::createRxHarqBuffer(src, userInfo);
}

template<class Base>
LteHarqBufferTx *D2dUeMacBase<Base>::createTxHarqBuffer(MacNodeId destId, Direction dir)
{
    // NOTE: unlike the base class, the UL buffer is paired with the MAC of destId, not of the serving cell
    if (dir == UL)
        return new LteHarqBufferTx(this->binder_, (unsigned int)this->harqProcesses_, this, check_and_cast<LteMacBase *>(this->binder_->getMacByNodeId(destId)));
    else // D2D or D2D_MULTI
        return new LteHarqBufferTxD2D(this->binder_, (unsigned int)this->harqProcesses_, this, check_and_cast<LteMacBase *>(this->binder_->getMacByNodeId(destId)));
}

template<class Base>
LcgScheduler *D2dUeMacBase<Base>::createLcgScheduler()
{
    return new LcgSchedulerD2D(this);
}

template<class Base>
void D2dUeMacBase<Base>::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        Base::handleMessage(msg);
        return;
    }

    auto pkt = check_and_cast<inet::Packet *>(msg);
    cGate *incoming = pkt->getArrivalGate();

    if (incoming == this->downInGate_) {
        auto userInfo = pkt->getTag<UserControlInfo>();

        if (userInfo->getFrameType() == D2DMODESWITCHPKT) {
            EV << "D2dUeMacBase::handleMessage - Received packet " << pkt->getName() <<
                " from port " << pkt->getArrivalGate()->getName() << endl;

            // message from phyIn gate (from the lower layer)
            this->emit(this->receivedPacketFromLowerLayerSignal_, pkt);

            // call handler
            macHandleD2DModeSwitch(pkt);

            return;
        }
    }

    Base::handleMessage(msg);
}

template<class Base>
void D2dUeMacBase<Base>::macHandleGrant(cPacket *pktAux)
{
    EV << NOW << " D2dUeMacBase::macHandleGrant - UE [" << this->nodeId_ << "] - Grant received " << endl;

    // extract grant
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto grant = pkt->popAtFront<LteSchedulingGrant>();

    auto userInfo = pkt->getTag<UserControlInfo>();
    GHz carrierFrequency = userInfo->getCarrierFrequency();
    EV << NOW << " D2dUeMacBase::macHandleGrant - Direction: " << dirToA(grant->getDirection()) << " Carrier: " << carrierFrequency << endl;

    // delete old grant
    if (this->schedulingGrant_.find(carrierFrequency) != this->schedulingGrant_.end() && this->schedulingGrant_[carrierFrequency] != nullptr) {
        this->schedulingGrant_[carrierFrequency] = nullptr;
    }

    // store received grant
    this->schedulingGrant_[carrierFrequency] = grant;
    if (grant->getPeriodic()) {
        this->periodCounter_[carrierFrequency] = grant->getPeriod();
        this->expirationCounter_[carrierFrequency] = grant->getExpiration();
    }

    EV << NOW << " Node " << this->nodeId_ << " received grant of blocks " << grant->getTotalGrantedBlocks()
       << ", bytes " << grant->getGrantedCwBytes(0) << " Direction: " << dirToA(grant->getDirection()) << endl;

    // clearing pending RAC requests
    this->racRequested_ = false;
    d2dUeHelper_.setRacD2DMulticastRequested(false);

    delete pkt;
}

template<class Base>
void D2dUeMacBase<Base>::checkRAC()
{
    EV << NOW << " D2dUeMacBase::checkRAC , Ue  " << this->nodeId_ << ", racTimer : " << this->racBackoffTimer_ << " maxRacTryOuts : " << this->maxRacTryouts_
       << ", raRespTimer:" << this->raRespTimer_ << endl;

    if (this->racBackoffTimer_ > 0) {
        this->racBackoffTimer_--;
        return;
    }

    if (this->raRespTimer_ > 0) {
        // decrease RAC response timer
        this->raRespTimer_--;
        EV << NOW << " D2dUeMacBase::checkRAC - waiting for previous RAC requests to complete (timer=" << this->raRespTimer_ << ")" << endl;
        return;
    }

    if (this->bsrRtxTimer_ > 0) {
        // decrease BSR timer
        this->bsrRtxTimer_--;
        EV << NOW << " D2dUeMacBase::checkRAC - waiting for a grant, BSR rtx timer has not expired yet (timer=" << this->bsrRtxTimer_ << ")" << endl;

        return;
    }

    // Avoids double requests within the same TTI window
    if (this->racRequested_) {
        EV << NOW << " D2dUeMacBase::checkRAC - double RAC request" << endl;
        this->racRequested_ = false;
        return;
    }
    if (d2dUeHelper_.getRacD2DMulticastRequested()) {
        EV << NOW << " D2dUeMacBase::checkRAC - double RAC request" << endl;
        d2dUeHelper_.setRacD2DMulticastRequested(false);
        return;
    }

    bool trigger = false;
    bool triggerD2DMulticast = false;

    for (auto [cid, connInfo] : this->connDescOut_) {
        if (!(connInfo.buffer->isEmpty())) {
            if (connInfo.flowInfo.getDirection() == D2D_MULTI)
                triggerD2DMulticast = true;
            else
                trigger = true;
            break;
        }
    }

    if (!trigger && !triggerD2DMulticast) {
        EV << NOW << " D2dUeMacBase::checkRAC , Ue " << this->nodeId_ << ", RAC aborted, no data in queues " << endl;
    }

    this->racRequested_ = trigger;
    bool racD2DMulticastRequested = d2dUeHelper_.getRacD2DMulticastRequested();
    if (!this->racRequested_)
        racD2DMulticastRequested = triggerD2DMulticast;
    d2dUeHelper_.setRacD2DMulticastRequested(racD2DMulticastRequested);
    if (this->racRequested_ || racD2DMulticastRequested) {
        auto pkt = new inet::Packet("RacRequest");
        GHz carrierFrequency = this->phy_->getPrimaryChannelModel()->getCarrierFrequency();
        pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFrequency);
        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(this->getMacNodeId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(this->getMacCellId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(RACPKT);

        auto racReq = inet::makeShared<LteRac>();
        racReq->setPreambleIndex(this->intuniform(0, this->numPreambles_ - 1));

        pkt->insertAtFront(racReq);
        this->sendLowerPackets(pkt);

        EV << NOW << " Ue  " << this->nodeId_ << " cell " << this->cellId_ << ", RAC request sent to PHY (preamble="
           << racReq->getPreambleIndex() << ")" << endl;

        // wait at least  "raRespWinStart_" TTIs before another RAC request
        this->raRespTimer_ = this->raRespWinStart_;
    }
}

template<class Base>
void D2dUeMacBase<Base>::macHandleRac(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto racPkt = pkt->peekAtFront<LteRac>();

    if (racPkt->getSuccess()) {
        EV << "D2dUeMacBase::macHandleRac - Ue " << this->nodeId_ << " won RAC" << endl;
        // if RAC is won, BSR has to be sent
        if (d2dUeHelper_.getRacD2DMulticastRequested())
            d2dUeHelper_.setBsrD2DMulticastTriggered(true);
        else
            this->bsrTriggered_ = true;

        // reset RAC counter
        this->currentRacTry_ = 0;
        //reset RAC backoff timer
        this->racBackoffTimer_ = 0;
    }
    else {
        // RAC has failed
        if (++this->currentRacTry_ >= this->maxRacTryouts_) {
            EV << NOW << " Ue " << this->nodeId_ << ", RAC reached max attempts : " << this->currentRacTry_ << endl;
            // no more RAC allowed
            //! TODO flush all buffers here
            //reset RAC counter
            this->currentRacTry_ = 0;
            //reset RAC backoff timer
            this->racBackoffTimer_ = 0;
        }
        else {
            // recompute backoff timer
            this->racBackoffTimer_ = this->uniform(this->minRacBackoff_, this->maxRacBackoff_);
            EV << NOW << " Ue " << this->nodeId_ << " RAC attempt failed, backoff extracted : " << this->racBackoffTimer_ << endl;
        }
    }
    delete pkt;
}

template<class Base>
void D2dUeMacBase<Base>::macHandleD2DModeSwitch(cPacket *pkt)
{
    d2dUeHelper_.macHandleD2DModeSwitch(pkt);
}

template<class Base>
void D2dUeMacBase<Base>::doHandover(MacNodeId targetEnb)
{
    if (targetEnb == NODEID_NONE)
        d2dUeHelper_.setEnb(nullptr);
    else {
        d2dUeHelper_.rebuildPreconfiguredTxParams(this->binder_);
        d2dUeHelper_.setEnb(check_and_cast<ID2dMacEnb *>(this->binder_->getMacByNodeId(targetEnb)));
    }
    Base::doHandover(targetEnb);
}

} //namespace

#endif
