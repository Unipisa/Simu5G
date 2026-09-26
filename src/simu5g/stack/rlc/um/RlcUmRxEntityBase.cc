//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/rlc/um/RlcUmRxEntityBase.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/rlc/RlcMux.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/mec/utils/MecCommon.h"

namespace simu5g {

using namespace inet;

// Registered here (single translation unit), in their historical order, so the
// signal-id assignment order is independent of the LTE/NR concrete .cc files.
simsignal_t RlcUmRxEntityBase::rlcDelaySignal_[2] = { cComponent::registerSignal("rlcDelayDl"), cComponent::registerSignal("rlcDelayUl") };
simsignal_t RlcUmRxEntityBase::rlcThroughputSignal_[2] = { cComponent::registerSignal("rlcThroughputDl"), cComponent::registerSignal("rlcThroughputUl") };
simsignal_t RlcUmRxEntityBase::rlcPduDelaySignal_[2] = { cComponent::registerSignal("rlcPduDelayDl"), cComponent::registerSignal("rlcPduDelayUl") };
simsignal_t RlcUmRxEntityBase::rlcPduThroughputSignal_[2] = { cComponent::registerSignal("rlcPduThroughputDl"), cComponent::registerSignal("rlcPduThroughputUl") };
simsignal_t RlcUmRxEntityBase::receivedPacketFromLowerLayerSignal_ = registerSignal("receivedPacketFromLowerLayer");
simsignal_t RlcUmRxEntityBase::sentPacketToUpperLayerSignal_ = registerSignal("sentPacketToUpperLayer");

void RlcUmRxEntityBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        LteMacBase *mac = getModuleFromPar<LteMacBase>(par("macModule"), this);
        ownerNodeId_ = mac->getMacNodeId();
        initMode(mac);

        WATCH(flowControlInfo_);
        WATCH(isBurst_);
        WATCH(totalBits_);
        WATCH(ttiBits_);
        WATCH(t1_);
        WATCH(t2_);
    }
}

void RlcUmRxEntityBase::handleBurst(BurstCheck event)
{
    // UL data-burst throughput (TS 136.314): track contiguous reception while the
    // RX buffer is continuously non-empty; report the burst on its end.
    simtime_t t1 = simTime();
    bool bufferEmpty = isEmpty();

    if (bufferEmpty) {
        if (isBurst_) {
            if ((t1_ - t2_) > TTI && rlcMux_ && flowControlInfo_) {
                Throughput throughput = { totalBits_, (t1_ - t2_) };
                rlcMux_->addUeThroughput(flowControlInfo_->getSourceId(), throughput);
            }
            totalBits_ = 0;
            t2_ = 0;
            t1_ = 0;
            isBurst_ = false;
        }
    }
    else {
        if (isBurst_) {
            if (event == ENQUE) {
                totalBits_ += ttiBits_;
                t1_ = t1;
            }
        }
        else {
            isBurst_ = true;
            totalBits_ = ttiBits_;
            t2_ = t1;
            t1_ = t1;
        }
    }
    ttiBits_ = 0;
}

} //namespace
