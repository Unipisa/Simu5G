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

#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/common/NetworkInterface.h>

#include "corenetwork/statsCollector/UeStatsCollector.h"
#include "stack/mac/LteMacBase.h"
#include "stack/packetFlowManager/PacketFlowManagerUe.h"
#include "stack/pdcp_rrc/LtePdcpRrc.h"

namespace simu5g {

Define_Module(UeStatsCollector);

void UeStatsCollector::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        collectorType_ = par("collectorType").stringValue();
    }
    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) { // same as lteMacUe, when reading the interface entry
        Binder *binder = inet::getModuleFromPar<Binder>(par("binderModule"), this);

        mac_.reference(this, "macModule", true);
        associateId_.value = binder->getIPv4Address(mac_->getMacNodeId()).str(); // UE_IPV4_ADDRESS
        associateId_.type = "1"; // UE_IPV4_ADDRESS

        /*
         * Get packetFlowManager if present.
         * When the UE has both Lte and NR NIC, two UeStatsCollector are created.
         * So each of them has to get the correct reference of the packetFlowManager,
         * since they are split, too.
         */

        bool isNr_ = (std::string(getContainingNicModule(mac_)->getComponentType()->getName()) == "NRNicUe");

        packetFlowManager_.reference(this, "packetFlowManagerModule", isNr_);

        handover_ = false;

        // packet delay
        ul_nongbr_delay_ue.init("ul_nongbr_delay_ue", par("delayPacketPeriods"), par("movingAverage"));
        dl_nongbr_delay_ue.init("dl_nongbr_delay_ue", par("delayPacketPeriods"), par("movingAverage"));
        // packet discard rate
        ul_nongbr_pdr_ue.init("ul_nongbr_pdr_ue", par("discardRatePeriods"), par("movingAverage"));
        dl_nongbr_pdr_ue.init("dl_nongbr_pdr_ue", par("discardRatePeriods"), par("movingAverage"));
        // scheduled throughput
        ul_nongbr_throughput_ue.init("ul_nongbr_throughput_ue", par("tPutPeriods"), par("movingAverage"));
        dl_nongbr_throughput_ue.init("dl_nongbr_throughput_ue", par("tPutPeriods"), par("movingAverage"));
        // data volume
        ul_nongbr_data_volume_ue.init("ul_nongbr_data_volume_ue", par("dataVolumePeriods"), par("movingAverage"));
        dl_nongbr_data_volume_ue.init("dl_nongbr_data_volume_ue", par("dataVolumePeriods"), par("movingAverage"));
    }
}

void UeStatsCollector::resetDelayCounter()
{
    if (packetFlowManager_ != nullptr)
        packetFlowManager_->resetDelayCounter();
}

void UeStatsCollector::add_ul_nongbr_delay_ue()
{
    if (packetFlowManager_ != nullptr) {
        double delay = packetFlowManager_->getDelayStats();
        if (delay != 0)
            EV << "UeStatsCollector::add_ul_nongbr_delay_ue() - delay: " << delay << endl;
        ul_nongbr_delay_ue.addValue((int)delay);
    }
}

// called by the eNodeBCollector
void UeStatsCollector::add_dl_nongbr_delay_ue(double value)
{
    dl_nongbr_delay_ue.addValue(value);
}

void UeStatsCollector::add_ul_nongbr_pdr_ue()
{
    DiscardedPkts pair = packetFlowManager_->getDiscardedPkt();
    double rate = ((double)pair.discarded * 1000000) / pair.total;
    ul_nongbr_pdr_ue.addValue((int)rate);
}

// called by the eNodeBCollector
void UeStatsCollector::add_dl_nongbr_pdr_ue(double value)
{
    dl_nongbr_pdr_ue.addValue(value);
}

// called by the eNodeBCollector
void UeStatsCollector::add_ul_nongbr_throughput_ue(double value)
{
    ul_nongbr_throughput_ue.addValue(value);
}

void UeStatsCollector::add_dl_nongbr_throughput_ue(double value)
{
    dl_nongbr_throughput_ue.addValue(value);
}

// called by the eNodeBCollector
void UeStatsCollector::add_ul_nongbr_data_volume_ue(unsigned int value)
{
    ul_nongbr_data_volume_ue.addValue(value);
}

void UeStatsCollector::add_dl_nongbr_data_volume_ue(unsigned int value)
{
    dl_nongbr_data_volume_ue.addValue(value);
}

int UeStatsCollector::get_ul_nongbr_delay_ue()
{
    return ul_nongbr_delay_ue.getMean();
}

int UeStatsCollector::get_dl_nongbr_delay_ue()
{
    return dl_nongbr_delay_ue.getMean();
}

int UeStatsCollector::get_ul_nongbr_pdr_ue()
{
    return ul_nongbr_pdr_ue.getMean();
}

int UeStatsCollector::get_dl_nongbr_pdr_ue()
{
    return dl_nongbr_pdr_ue.getMean();
}

int UeStatsCollector::get_ul_nongbr_throughput_ue()
{
    return ul_nongbr_throughput_ue.getMean();
}

int UeStatsCollector::get_dl_nongbr_throughput_ue()
{
    return dl_nongbr_throughput_ue.getMean();
}

int UeStatsCollector::get_ul_nongbr_data_volume_ue()
{
    return ul_nongbr_data_volume_ue.getMean();
}

int UeStatsCollector::get_dl_nongbr_data_volume_ue()
{
    return dl_nongbr_data_volume_ue.getMean();
}

DiscardedPkts UeStatsCollector::getULDiscardedPkt()
{
    DiscardedPkts pair = { 0, 0 };
    if (packetFlowManager_ != nullptr) {

        pair = packetFlowManager_->getDiscardedPkt();
        //double rate = ((double)pair.discarded * 1000000) / pair.total;
    }
    return pair;
}

void UeStatsCollector::resetStats()
{
    packetFlowManager_->clearStats();
    // packet delay
    ul_nongbr_delay_ue.reset();
    dl_nongbr_delay_ue.reset();
    // packet discard rate
    ul_nongbr_pdr_ue.reset();
    dl_nongbr_pdr_ue.reset();
    // scheduled throughput
    ul_nongbr_throughput_ue.reset();
    dl_nongbr_throughput_ue.reset();
    // data volume
    ul_nongbr_data_volume_ue.reset();
    dl_nongbr_data_volume_ue.reset();
}

} //namespace

