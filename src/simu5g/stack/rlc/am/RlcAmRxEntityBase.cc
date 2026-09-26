//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/common/NetworkInterface.h>

#include "simu5g/stack/rlc/am/RlcAmRxEntityBase.h"

namespace simu5g {

using namespace inet;

// LTE statistics
simsignal_t RlcAmRxEntityBase::rlcPacketLossSignal_[2] = { registerSignal("rlcPacketLossDl"), registerSignal("rlcPacketLossUl") };
simsignal_t RlcAmRxEntityBase::rlcPduPacketLossSignal_[2] = { registerSignal("rlcPduPacketLossDl"), registerSignal("rlcPduPacketLossUl") };
simsignal_t RlcAmRxEntityBase::rlcDelaySignal_[2] = { registerSignal("rlcDelayDl"), registerSignal("rlcDelayUl") };
simsignal_t RlcAmRxEntityBase::rlcThroughputSignal_[2] = { registerSignal("rlcThroughputDl"), registerSignal("rlcThroughputUl") };
simsignal_t RlcAmRxEntityBase::rlcPduDelaySignal_[2] = { registerSignal("rlcPduDelayDl"), registerSignal("rlcPduDelayUl") };
simsignal_t RlcAmRxEntityBase::rlcPduThroughputSignal_[2] = { registerSignal("rlcPduThroughputDl"), registerSignal("rlcPduThroughputUl") };

// NR statistics (declared only by the NrRlcAmRxEntity profile)
simsignal_t RlcAmRxEntityBase::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t RlcAmRxEntityBase::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");
simsignal_t RlcAmRxEntityBase::rxWindowOccupationSignal_ = registerSignal("rxWindowOccupation");

void RlcAmRxEntityBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        initMode();

        WATCH(flowControlInfo_);
        WATCH(ackFlowControlInfo_);
        WATCH(lastSentAck_);
        WATCH(totalRcvdBytes_);
        WATCH(totalPduRcvdBytes_);
    }
}

void RlcAmRxEntityBase::emitRxStatistics(bool perPdu, double throughput, simtime_t delay)
{
    if (ackFlowControlInfo_ == nullptr)
        return;

    // ackFlowControlInfo_ is the reversed flow info (it stamps the STATUS PDUs going
    // back), so the data flowed in the opposite direction to the one it names.
    Direction dir = (ackFlowControlInfo_->getDirection() == DL) ? UL : DL;

    emit(perPdu ? rlcPduThroughputSignal_[dir] : rlcThroughputSignal_[dir], throughput);
    emit(perPdu ? rlcPduDelaySignal_[dir] : rlcDelaySignal_[dir], delay.dbl());
    if (!perPdu) {
        // AM repairs everything it is given, so nothing is ever lost to the upper
        // layer. Reporting the zero makes that a measured result rather than an
        // absent one.
        emit(rlcPacketLossSignal_[dir], 0.0);
    }
}

} //namespace
