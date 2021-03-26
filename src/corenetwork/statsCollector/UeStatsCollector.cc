//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "corenetwork/statsCollector/UeStatsCollector.h"
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"
#include "stack/mac/layer/LteMacUe.h"
#include "inet/common/ModuleAccess.h"
#include "stack/packetFlowManager/PacketFlowManagerUe.h"

Define_Module(UeStatsCollector);

UeStatsCollector::UeStatsCollector()
{
    pdcp_ = nullptr;
    mac_ = nullptr;
    flowManager_ = nullptr;

}

void UeStatsCollector::initialize(int stage){
    if (stage == inet::INITSTAGE_APPLICATION_LAYER) // same as lteMacUe, when read the interface entry
    {
        LteBinder* binder = getBinder();


        mac_ = check_and_cast<LteMacUe *>(getParentModule()->getSubmodule("lteNic")->getSubmodule("mac"));
        pdcp_ = check_and_cast<LtePdcpRrcUe *>(getParentModule()->getSubmodule("lteNic")->getSubmodule("pdcpRrc"));

        associateId_.value = binder->getIPv4Address(mac_->getMacNodeId()).str(); // UE_IPV4_ADDRESS
        associateId_.type = "1"; // UE_IPV4_ADDRESS


        // get packetFlowManager if present
        if(getParentModule()->getSubmodule("lteNic")->findSubmodule("packetFlowManager") != -1)
            flowManager_ = check_and_cast<PacketFlowManagerUe *>(getParentModule()->getSubmodule("lteNic")->getSubmodule("packetFlowManager"));

        handover_ = false;

        // packet delay
         ul_nongbr_delay_ue.init("ul_nongbr_delay_ue", par("delayPacketPeriods"), par("movingAverage"));
         dl_nongbr_delay_ue.init("dl_nongbr_delay_ue",par("delayPacketPeriods"), par("movingAverage"));
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
    if(flowManager_ != nullptr)
        flowManager_ ->resetDelayCounter();
}

void UeStatsCollector::add_ul_nongbr_delay_ue()
{
    if(flowManager_ != nullptr)
    {
        double delay = flowManager_->getDelayStats();
        if(delay != 0)
            EV << "UeStatsCollector::add_ul_nongbr_delay_ue() - delay: " << delay << endl;
        ul_nongbr_delay_ue.addValue(delay);
    }
}

// called by the eNodeBCollector
void UeStatsCollector::add_dl_nongbr_delay_ue(double value)
{
    dl_nongbr_delay_ue.addValue(value);
}

void UeStatsCollector::add_ul_nongbr_pdr_ue(double value)
{
    ul_nongbr_pdr_ue.addValue(value);
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
    DiscardedPkts pair = {0,0};
    if(flowManager_ != nullptr)
    {

        pair = flowManager_->getDiscardedPkt();
        double rate = ((double)pair.discarded * 1000000) / pair.total;
        add_ul_nongbr_pdr_ue(rate);
    }
    return pair;
}
