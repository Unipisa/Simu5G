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

#ifndef _SIMU5G_RLCUMENTITYD2D_H_
#define _SIMU5G_RLCUMENTITYD2D_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/stack/d2d/rlc/ID2dRlcUmTxEntity.h"
#include "simu5g/stack/d2d/rrc/D2DModeController.h"
#include "simu5g/stack/rlc/um/LteRlcUmTxEntity.h"
#include "simu5g/stack/rlc/um/NrRlcUmTxEntity.h"
#include "simu5g/stack/rlc/um/LteRlcUmRxEntity.h"
#include "simu5g/stack/rlc/um/NrRlcUmRxEntity.h"
#include "simu5g/stack/rlc/packet/LteRlcNewDataTag_m.h"

namespace simu5g {

using namespace omnetpp;

// =====================================================================
// TX side
// =====================================================================

/**
 * @brief D2D mode-switch machinery for an RLC UM transmitting entity.
 *
 * Mixin over either UM TX profile (Base = LteRlcUmTxEntity or
 * NrRlcUmTxEntity). It owns every piece of state the mode switch needs, so the
 * core entities carry none of it:
 *
 *  - while the entity of the *newly selected* mode waits for the *old* mode's
 *    entity to drain, incoming SDUs are parked in a holding queue
 *    (interceptSdu()) instead of being buffered and announced to the MAC;
 *  - the old-mode entity raises notifyEmptyBuffer_ and, once its TX buffer runs
 *    dry (onTxBufferEmptied()), tells the ~D2DModeController to release the
 *    peer's holding queue.
 *
 * The two profiles differ only in the buffer primitives, which are reached
 * through seams the core already declares -- storeSdu(), clearQueue(),
 * isTxBufferEmpty() -- plus resetTxNumbering(), left to the leaf.
 */
template<class Base>
class RlcUmTxEntityD2D : public Base, public ID2dRlcUmTxEntity
{
  protected:
    // D2D mode-switch controller of this NIC (absent on non-D2D stacks)
    inet::ModuleRefByPar<D2DModeController> d2dModeController_;

    // peer this entity is currently registered under at the controller, so that it can
    // withdraw itself under exactly the key it was filed under (see ~RlcUmTxEntityD2D)
    MacNodeId registeredPeerId_ = NODEID_NONE;

    // if true, the entity watches for its TX buffer becoming empty
    bool notifyEmptyBuffer_ = false;

    // if true, incoming SDUs are parked in sduHoldingQueue_ instead of buffered
    bool holdingDownstreamInPackets_ = false;

    // the SDU holding buffer
    inet::cPacketQueue sduHoldingQueue_;

    void initialize(int stage) override
    {
        Base::initialize(stage);
        if (stage == inet::INITSTAGE_LOCAL) {
            d2dModeController_.reference(this, "d2dModeControllerModule", false);
            WATCH(registeredPeerId_);
            WATCH(notifyEmptyBuffer_);
            WATCH(holdingDownstreamInPackets_);
            WATCH(sduHoldingQueue_);
        }
    }

    void setFlowControlInfo(FlowControlInfo *info) override
    {
        Base::setFlowControlInfo(info);

        // D2D peer tracking: register with the mode controller now that the peer id
        // is known, so mode switches can coordinate holding/draining of the buffers
        // (this used to be done by BearerManagement for every UM entity).
        if (d2dModeController_) {
            unregisterFromD2DModeController();  // no-op unless the flow is being re-set
            registeredPeerId_ = MacNodeId(this->flowControlInfo_->getD2dRxPeerId());
            d2dModeController_->registerD2DPeerTxEntity(registeredPeerId_, this);
        }
    }

    /**
     * Withdraw from the controller's per-peer registry. That registry holds raw pointers to
     * these entity modules, and BearerManagement::deleteLocalRlcQueues() deletes them on
     * handover and on bearer teardown, so an entity that goes away without withdrawing
     * leaves a dangling pointer behind -- which the next registration for the same peer then
     * calls isEmptyingBuffer() on. The ModuleRefByPar is null once the controller itself is
     * gone, which is the case when the whole NIC is being torn down.
     */
    void unregisterFromD2DModeController()
    {
        if (d2dModeController_ && registeredPeerId_ != NODEID_NONE)
            d2dModeController_->unregisterD2DPeerTxEntity(registeredPeerId_, this);
        registeredPeerId_ = NODEID_NONE;
    }

    // park the SDU while the old-mode entity is still draining
    bool interceptSdu(inet::Packet *pkt) override
    {
        if (holdingDownstreamInPackets_) {
            // do not store in the TX buffer and do not signal the MAC layer
            EV << NOW << " RlcUmTxEntityD2D::interceptSdu - storing new SDU into the holding buffer" << endl;
            sduHoldingQueue_.insert(pkt);
            return true;
        }
        return Base::interceptSdu(pkt);
    }

    // the old-mode entity has drained: let the peer's new-mode entity go ahead
    void onTxBufferEmptied() override
    {
        if (notifyEmptyBuffer_ && this->isTxBufferEmpty()) {
            notifyEmptyBuffer_ = false;
            if (d2dModeController_ && this->flowControlInfo_)
                d2dModeController_->resumeDownstreamInPackets(this->flowControlInfo_->getD2dRxPeerId());
        }
    }

    /**
     * Reset the UM transmit sequence numbering of the new-mode entity, so that
     * its numbering restarts in step with the peer's receiving side.
     * FI keeps its own PDU SN counter, SO keeps TX_Next in the buffer.
     */
    virtual void resetTxNumbering() = 0;

  public:
    ~RlcUmTxEntityD2D() override { unregisterFromD2DModeController(); }

    void startHoldingDownstreamInPackets() override { holdingDownstreamInPackets_ = true; }

    bool isHoldingDownstreamInPackets() override { return holdingDownstreamInPackets_; }

    bool isEmptyingBuffer() override { return notifyEmptyBuffer_; }

    void resumeDownstreamInPackets() override
    {
        EV << NOW << " RlcUmTxEntityD2D::resumeDownstreamInPackets - resume buffering incoming downstream packets of the RLC entity associated with the new mode" << endl;

        holdingDownstreamInPackets_ = false;

        // move all SDUs in the holding buffer to the TX buffer
        while (!sduHoldingQueue_.isEmpty()) {
            auto pktRlc = check_and_cast<inet::Packet *>(sduHoldingQueue_.front());
            sduHoldingQueue_.pop();

            // store the SDU in the TX buffer and notify the MAC (drop on overflow)
            this->bufferSduAndNotifyMac(pktRlc);
        }
    }

    void rlcHandleD2DModeSwitch(bool oldConnection, bool clearBuffer) override
    {
        // Clearing the old-mode buffer deletes SDUs owned by THIS entity, and we are
        // reached by a direct call from the mux; switch context so those deletes run
        // in the owner's context.
        Enter_Method_Silent("rlcHandleD2DModeSwitch()");

        if (oldConnection) {
            if (getNodeTypeById(this->ownerNodeId_) == NODEB) {
                EV << NOW << " RlcUmTxEntityD2D::rlcHandleD2DModeSwitch - nothing to do on DL leg of IM flow" << endl;
                return;
            }

            if (clearBuffer) {
                EV << NOW << " RlcUmTxEntityD2D::rlcHandleD2DModeSwitch - clear TX buffer of the RLC entity associated with the old mode" << endl;
                this->clearQueue();
            }
            else if (!this->isTxBufferEmpty()) {
                EV << NOW << " RlcUmTxEntityD2D::rlcHandleD2DModeSwitch - check when the TX buffer of the old mode becomes empty" << endl;
                notifyEmptyBuffer_ = true;
            }
            else {
                EV << NOW << " RlcUmTxEntityD2D::rlcHandleD2DModeSwitch - TX buffer of the old mode is already empty" << endl;
            }
        }
        else {
            EV << NOW << " RlcUmTxEntityD2D::rlcHandleD2DModeSwitch - reset numbering of the RLC TX entity corresponding to the new mode" << endl;
            resetTxNumbering();

            if (!clearBuffer && d2dModeController_ && this->flowControlInfo_ &&
                    d2dModeController_->isEmptyingTxBuffer(this->flowControlInfo_->getD2dRxPeerId())) {
                // the old-mode entity is still draining: hold incoming SDUs until it finishes
                EV << NOW << " RlcUmTxEntityD2D::rlcHandleD2DModeSwitch - halt incoming downstream connections of the new mode" << endl;
                startHoldingDownstreamInPackets();
            }
        }
    }
};

/**
 * @brief D2D-capable LTE (TS 36.322, FI framing) RLC UM transmitting entity.
 */
class LteRlcUmTxEntityD2D : public RlcUmTxEntityD2D<LteRlcUmTxEntity>
{
  protected:
    void resetTxNumbering() override { sno_ = 0; }
};

/**
 * @brief D2D-capable NR (TS 38.322, SO framing) RLC UM transmitting entity.
 */
class NrRlcUmTxEntityD2D : public RlcUmTxEntityD2D<NrRlcUmTxEntity>
{
  protected:
    void resetTxNumbering() override { sduBuffer->resetTxNext(); }
};

// =====================================================================
// RX side
// =====================================================================

/**
 * @brief D2D mode-switch machinery for an RLC UM receiving entity.
 *
 * Mixin over either UM RX profile (Base = LteRlcUmRxEntity or
 * NrRlcUmRxEntity). It carries the mode-switch skeleton and the D2D branch of
 * the direction-indexed statistics; discarding and renumbering the RX buffer is
 * profile-specific and left to the leaf.
 */
template<class Base>
class RlcUmRxEntityD2D : public Base, public ID2dRlcUmRxEntity
{
  protected:
    // returns true if this entity serves a D2D_MULTI connection
    bool isD2DMultiConnection() { return this->flowControlInfo_->getDirection() == D2D_MULTI; }

    // Interned in initialize() rather than registered by a static initializer, so that
    // linking the D2D package in cannot shift the signal ids the core assigns. See the
    // "Signals" note in D2dUeMacBase.h.
    simsignal_t rlcDelayD2DSignal_ = SIMSIGNAL_NULL;
    simsignal_t rlcThroughputD2DSignal_ = SIMSIGNAL_NULL;
    simsignal_t rlcPduDelayD2DSignal_ = SIMSIGNAL_NULL;
    simsignal_t rlcPduThroughputD2DSignal_ = SIMSIGNAL_NULL;

    void initialize(int stage) override
    {
        Base::initialize(stage);
        if (stage == inet::INITSTAGE_LOCAL) {
            rlcDelayD2DSignal_ = cComponent::registerSignal("rlcDelayD2D");
            rlcThroughputD2DSignal_ = cComponent::registerSignal("rlcThroughputD2D");
            rlcPduDelayD2DSignal_ = cComponent::registerSignal("rlcPduDelayD2D");
            rlcPduThroughputD2DSignal_ = cComponent::registerSignal("rlcPduThroughputD2D");
        }
    }

    /**
     * Discard whatever the old mode left in the RX buffer (delivering what can
     * still be reassembled) and stop the reassembly/reordering timer.
     */
    virtual void discardRxBufferForModeSwitch() = 0;

    /**
     * Reset the receive numbering of the new-mode entity, and arrange for its
     * first PDU to be taken as in-sequence.
     */
    virtual void resetRxNumbering() = 0;

  public:
    void rlcHandleD2DModeSwitch(bool oldConnection, bool oldMode, bool clearBuffer) override
    {
        Enter_Method_Silent("rlcHandleD2DModeSwitch()");

        if (oldConnection) {
            if (getNodeTypeById(this->ownerNodeId_) == UE && oldMode == IM) {
                EV << NOW << " RlcUmRxEntityD2D::rlcHandleD2DModeSwitch - nothing to do on DL leg of IM flow" << endl;
                return;
            }

            if (clearBuffer) {
                EV << NOW << " RlcUmRxEntityD2D::rlcHandleD2DModeSwitch - clear RX buffer of the RLC entity associated with the old mode" << endl;
                discardRxBufferForModeSwitch();
            }
        }
        else {
            EV << NOW << " RlcUmRxEntityD2D::rlcHandleD2DModeSwitch - handle numbering of the RLC entity associated with the newly selected mode" << endl;
            resetRxNumbering();
        }
    }
};

/**
 * @brief D2D-capable LTE (TS 36.322) RLC UM receiving entity.
 *
 * Adds, on top of the mode-switch skeleton: the D2D_MULTI reordering-window
 * special case, the post-mode-switch reassembly resynchronisation, and the D2D
 * branch of the throughput/delay statistics.
 */
class LteRlcUmRxEntityD2D : public RlcUmRxEntityD2D<LteRlcUmRxEntity>
{
  protected:
    // after a mode switch, the first PDU of the new-mode entity is forced in-sequence
    bool resetFlag_ = false;

    void initialize(int stage) override
    {
        RlcUmRxEntityD2D<LteRlcUmRxEntity>::initialize(stage);
        if (stage == inet::INITSTAGE_LOCAL)
            WATCH(resetFlag_);
    }

    void discardRxBufferForModeSwitch() override;
    void resetRxNumbering() override;

    void onFirstPduEnqueued(unsigned int pduSno) override;
    bool consumeReassemblyReset(unsigned int pduSno) override;
    void emitPduStats(Direction dir, double tputSample, simtime_t creationTime) override;
    void emitSduStats(Direction dir, double tputSample, simtime_t creationTime) override;
};

/**
 * @brief D2D-capable NR (TS 38.322) RLC UM receiving entity.
 */
class NrRlcUmRxEntityD2D : public RlcUmRxEntityD2D<NrRlcUmRxEntity>
{
  protected:
    void discardRxBufferForModeSwitch() override;
    void resetRxNumbering() override;

    void emitRxStatistics(bool perPdu, double throughput, simtime_t delay) override;
};

} //namespace

#endif
