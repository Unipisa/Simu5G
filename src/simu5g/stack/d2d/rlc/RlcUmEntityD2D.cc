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

#include "simu5g/stack/d2d/rlc/RlcUmEntityD2D.h"

namespace simu5g {

// All four D2D UM leaves live in this single translation unit, as the D2D
// channel-model leaves do: the mixins are templates, so this gives de-facto
// explicit instantiation and one place to reason about registration order.
Define_Module(LteRlcUmTxEntityD2D);
Define_Module(NrRlcUmTxEntityD2D);
Define_Module(LteRlcUmRxEntityD2D);
Define_Module(NrRlcUmRxEntityD2D);

// The D2D statistic signals emitted below are owned by RlcUmRxEntityD2D<Base> and
// interned at runtime in its initialize() -- see the "Signals" note in D2dUeMacBase.h.

using namespace inet;

// =====================================================================
// LteRlcUmRxEntityD2D
// =====================================================================

void LteRlcUmRxEntityD2D::discardRxBufferForModeSwitch()
{
    // deliver whatever is still reassemblable from the old mode
    for (unsigned int i = 0; i < rxWindowDesc_.windowSize_; i++)
        reassemble(i);

    pduBuffer_.clear();

    for (auto&& i : received_)
        i = false;

    clearBufferedSdu();

    if (t_reordering_.busy())
        t_reordering_.stop();
}

void LteRlcUmRxEntityD2D::resetRxNumbering()
{
    rxWindowDesc_.clear();
    resetFlag_ = true;
}

void LteRlcUmRxEntityD2D::onFirstPduEnqueued(unsigned int pduSno)
{
    if (!init_ && isD2DMultiConnection()) {
        // for D2D multicast connections, the first received PDU must be considered as the first valid PDU
        rxWindowDesc_.clear(pduSno);
        // setting the window size to 1 allows the entity to deliver immediately out-of-sequence SDU,
        // since reordering is not applicable for D2D multicast communications
        rxWindowDesc_.windowSize_ = 1;
        init_ = true;
    }
}

bool LteRlcUmRxEntityD2D::consumeReassemblyReset(unsigned int pduSno)
{
    if (!resetFlag_)
        return false;

    // by doing this, the arrived PDU and its first extracted SDU will be considered in order.
    // This helps to retrieve the synchronization between SNs at the tx and rx after a mode switch.
    lastPduReassembled_ = pduSno - 1;
    resetFlag_ = false;
    return true;
}

void LteRlcUmRxEntityD2D::emitPduStats(Direction dir, double tputSample, simtime_t creationTime)
{
    if (dir == D2D || dir == D2D_MULTI) { // UE in DM
        emit(rlcPduThroughputD2DSignal_, tputSample);
        emit(rlcPduDelayD2DSignal_, (NOW - creationTime).dbl());
    }
    else { // UE in IM
        LteRlcUmRxEntity::emitPduStats(dir, tputSample, creationTime);
    }
}

void LteRlcUmRxEntityD2D::emitSduStats(Direction dir, double tputSample, simtime_t arrivalTime)
{
    if (dir == D2D || dir == D2D_MULTI) { // UE in DM
        emit(rlcThroughputD2DSignal_, tputSample);
        emit(rlcDelayD2DSignal_, (NOW - arrivalTime).dbl());
    }
    else { // UE in IM
        LteRlcUmRxEntity::emitSduStats(dir, tputSample, arrivalTime);
    }
}

// =====================================================================
// NrRlcUmRxEntityD2D
// =====================================================================

void NrRlcUmRxEntityD2D::discardRxBufferForModeSwitch()
{
    // discard any partially-reassembled SDUs of the old mode
    sduBuffer->clearBuffer();
    sduBuffer->reset();
    if (t_ReassemblyTimer->isScheduled())
        cancelEvent(t_ReassemblyTimer);
}

void NrRlcUmRxEntityD2D::resetRxNumbering()
{
    // new-mode entity: reset the SN window. Unlike the FI profile, SO reassembly
    // is byte-offset driven and needs no first-PDU resynchronisation flag.
    sduBuffer->reset();
}

void NrRlcUmRxEntityD2D::emitRxStatistics(bool perPdu, double throughput, simtime_t delay)
{
    Direction dir = static_cast<Direction>(flowControlInfo_->getDirection());
    if (dir == D2D || dir == D2D_MULTI) {
        emit(perPdu ? rlcPduThroughputD2DSignal_ : rlcThroughputD2DSignal_, throughput);
        emit(perPdu ? rlcPduDelayD2DSignal_ : rlcDelayD2DSignal_, delay.dbl());
    }
    else {
        NrRlcUmRxEntity::emitRxStatistics(perPdu, throughput, delay);
    }
}

} //namespace
