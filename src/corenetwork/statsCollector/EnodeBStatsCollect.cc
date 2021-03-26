//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "corenetwork/statsCollector/EnodeBStatsCollector.h"
#include "corenetwork/statsCollector/UeStatsCollector.h"
#include "stack/packetFlowManager/PacketFlowManagerEnb.h"
#include "stack/rlc/um/LteRlcUm.h"
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"
#include "stack/mac/layer/LteMacEnb.h"
#include <string>

Define_Module(EnodeBStatsCollector);
EnodeBStatsCollector::EnodeBStatsCollector()
{
    rlc_ = nullptr;
    packetDelay_ = nullptr;
    mac_ = nullptr;
    pdcp_ = nullptr;
    flowManager_ = nullptr;
}


EnodeBStatsCollector::~EnodeBStatsCollector()
{
    cancelAndDelete(pdcpBytes_);
    cancelAndDelete(prbUsage_);
    cancelAndDelete(discardRate_);
    cancelAndDelete(activeUsers_);
    cancelAndDelete(packetDelay_);
    cancelAndDelete(tPut_);

}

void EnodeBStatsCollector::initialize(int stage){
    if (stage == inet::INITSTAGE_APPLICATION_LAYER)//inet::INITSTAGE_LOCAL)
    {
        EV << "EnodeBStatsCollector::initialize stage: "<< stage << endl;

        ecgi_.plmn.mcc = getAncestorPar("mcc").stdstringValue();
        ecgi_.plmn.mnc = getAncestorPar("mnc").stdstringValue();

        mac_ = check_and_cast<LteMacEnb *>(getParentModule()->getSubmodule("lteNic")->getSubmodule("mac"));
        pdcp_ = check_and_cast<LtePdcpRrcEnb *>(getParentModule()->getSubmodule("lteNic")->getSubmodule("pdcpRrc"));

        cModule *rlc = getParentModule()->getSubmodule("lteNic")->getSubmodule("rlc");
        if(rlc->findSubmodule("um") != -1)
        {
            rlc_ = check_and_cast<LteRlcUm *>(getParentModule()->getSubmodule("lteNic")->getSubmodule("rlc")->getSubmodule("um"));
        }
        else
        {
            throw cRuntimeError("EnodeBStatsCollector::initialize - EnodeB statistic collector only works with RLC in UM mode");
        }
        
        if(getParentModule()->getSubmodule("lteNic")->findSubmodule("packetFlowManager") != -1)
            flowManager_ = check_and_cast<PacketFlowManagerEnb *>(getParentModule()->getSubmodule("lteNic")->getSubmodule("packetFlowManager"));

        cellInfo_ = check_and_cast<LteCellInfo *>(getParentModule()->getSubmodule("cellInfo"));
        ecgi_.cellId = cellInfo_->getMacCellId();

        dl_total_prb_usage_cell.init("dl_total_prb_usage_cell", par("prbUsagePeriods"), par("movingAverage"));
        ul_total_prb_usage_cell.init("ul_total_prb_usage_cell", par("prbUsagePeriods"), par("movingAverage"));

        number_of_active_ue_dl_nongbr_cell.init("number_of_active_ue_dl_nongbr_cell", par("activeUserPeriods"), false);
        number_of_active_ue_ul_nongbr_cell.init("number_of_active_ue_ul_nongbr_cell", par("activeUserPeriods"), false);

        dl_nongbr_pdr_cell.init("dl_nongbr_pdr_cell", par("discardRatePeriods"), false);
        ul_nongbr_pdr_cell.init("ul_nongbr_pdr_cell", par("discardRatePeriods"), false);

        // setup timer
        prbUsage_    = new cMessage("prbUsage_");
        activeUsers_ = new cMessage("activeUsers_");
        discardRate_ = new cMessage("discardRate_");
        packetDelay_ = new cMessage("packetDelay_");
        pdcpBytes_   = new cMessage("pdcpBytes_");
        tPut_        = new cMessage("tPut_");

        prbUsagePeriod_    = par("prbUsagePeriod");
        activeUsersPeriod_ = par("activeUserPeriod");
        discardRatePeriod_ = par("discardRatePeriod");
        delayPacketPeriod_ = par("delayPacketPeriod");
        dataVolumePeriod_  = par("dataVolumePeriod");
        tPutPeriod_        = par("tPutPeriod");

        // start scheduling the l2 meas
        // schedule only stats not using packetFlowManager

        scheduleAt(NOW + prbUsagePeriod_, prbUsage_);
        scheduleAt(NOW + activeUsersPeriod_, activeUsers_);
        scheduleAt(NOW + dataVolumePeriod_, pdcpBytes_);
        if(flowManager_ != nullptr)
        {
            scheduleAt(NOW + discardRatePeriod_, discardRate_);
            scheduleAt(NOW + delayPacketPeriod_, packetDelay_);
            scheduleAt(NOW + tPutPeriod_,tPut_);
        }
    }
}


void EnodeBStatsCollector::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage())
    {
        EV << "EnodeBStatsCollector::handleMessage - get " << msg->getName() << "statistics" << endl;

        if(strcmp(msg->getName(),"prbUsage_") == 0)
        {
            add_dl_total_prb_usage_cell();
            add_ul_total_prb_usage_cell();
            scheduleAt(NOW + prbUsagePeriod_, prbUsage_);
        }
        else if(strcmp(msg->getName(), "activeUsers_") == 0)
        {
            add_number_of_active_ue_dl_nongbr_cell();
            add_number_of_active_ue_ul_nongbr_cell();
            scheduleAt(NOW + activeUsersPeriod_, activeUsers_);

        }
        else if(strcmp(msg->getName(), "tPut_") == 0)
        {
            add_dl_nongbr_throughput_ue_perUser();
            add_ul_nongbr_throughput_ue_perUser();
            scheduleAt(NOW + tPutPeriod_, tPut_);

        }
        else if(strcmp(msg->getName(), "discardRate_") == 0)
        {
            add_dl_nongbr_pdr_cell();
            add_ul_nongbr_pdr_cell();

            // add packet discard rate stats for each user
            add_dl_nongbr_pdr_cell_perUser();
            // ul is done add_ul_nongbr_pdr_cell

            //reset counters
            flowManager_->resetDiscardCounter();
            resetDiscardCounterPerUe();
            scheduleAt(NOW + discardRatePeriod_, discardRate_);

        }
        else if(strcmp(msg->getName(), "packetDelay_") == 0)
        {
            add_dl_nongbr_delay_perUser();
            add_ul_nongbr_delay_perUser();

            //reset counter
            resetDelayCounterPerUe();
            scheduleAt(NOW + delayPacketPeriod_, packetDelay_);
        }
        else if(strcmp(msg->getName(), "pdcpBytes_") == 0)
        {
            add_ul_nongbr_data_volume_ue_perUser();
            add_dl_nongbr_data_volume_ue_perUser();
            resetBytesCountersPerUe();
            scheduleAt(NOW + dataVolumePeriod_, pdcpBytes_);
        }
        else
        {
          delete msg;
        }

    }
    else
        delete msg;
}


void EnodeBStatsCollector::resetDiscardCounterPerUe()
{
    EV << "EnodeBStatsCollector::resetDiscardCounterPerUe " << endl;
    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
    UeStatsCollectorMap::iterator end = ueCollectors_.end();
    for(; it != end ; ++it)
    {
        flowManager_->resetDiscardCounterPerUe(it->first);
    }
}

void EnodeBStatsCollector::resetDelayCounterPerUe()
{
    EV << "EnodeBStatsCollector::resetDelayCounterPerUe " << endl;

    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
    UeStatsCollectorMap::iterator end = ueCollectors_.end();
    for(; it != end ; ++it)
    {
        flowManager_->resetDelayCounterPerUe(it->first);
        it->second->resetDelayCounter();
    }
}


void EnodeBStatsCollector::resetThroughputCountersPerUe()
{
    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
        UeStatsCollectorMap::iterator end = ueCollectors_.end();
        for(; it != end ; ++it)
        {
            flowManager_->resetThroughputCounterPerUe(it->first);
        }
}

void EnodeBStatsCollector::resetBytesCountersPerUe()
{
    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
    UeStatsCollectorMap::iterator end = ueCollectors_.end();
    for(; it != end ; ++it)
    {
//        pdcp_->resetPdcpBytesDlPerUe(it->first);
//        pdcp_->resetPdcpBytesUlPerUe(it->first);
    }
}


// UeStatsCollector management methods
void EnodeBStatsCollector::addUeCollector(MacNodeId id, UeStatsCollector* ueCollector)
{
    if(ueCollectors_.find(id) == ueCollectors_.end())
    {
        ueCollectors_.insert(std::pair<MacNodeId,UeStatsCollector *>(id, ueCollector));
    }
    else
    {
        throw cRuntimeError("EnodeBStatsCollector::addUeCollector - UeStatsCollector already present for UE nodeid[%d]", id);
    }
}

void EnodeBStatsCollector::removeUeCollector(MacNodeId id)
{
    std::map<MacNodeId, UeStatsCollector*>::iterator it = ueCollectors_.find(id);
    if(it != ueCollectors_.end())
    {
        ueCollectors_.erase(it);
    }
    else
    {
        throw cRuntimeError("EnodeBStatsCollector::removeUeCollector - UeStatsCollector not present for UE nodeid[%d]", id);
    }
}

UeStatsCollector* EnodeBStatsCollector::getUeCollector(MacNodeId id)
{
    std::map<MacNodeId, UeStatsCollector*>::iterator it = ueCollectors_.find(id);
    if(it != ueCollectors_.end())
    {
       return it->second;
    }
    else
    {
       throw cRuntimeError("EnodeBStatsCollector::removeUeCollector - UeStatsCollector not present for UE nodeid[%d]", id);
    }
}

UeStatsCollectorMap* EnodeBStatsCollector::getCollectorMap()
{
    return &ueCollectors_;
}

bool EnodeBStatsCollector::hasUeCollector(MacNodeId id)
{
    return (ueCollectors_.find(id) != ueCollectors_.end()) ? true : false;
}

void EnodeBStatsCollector::add_dl_total_prb_usage_cell()
{
//    double prb_usage = mac_->getUtilization(DL);
//    EV << "EnodeBStatsCollector::add_dl_total_prb_usage_cell " << prb_usage << "%"<< endl;
//    dl_total_prb_usage_cell.addValue(prb_usage);
}

void EnodeBStatsCollector::add_ul_total_prb_usage_cell()
{
//    double prb_usage = mac_->getUtilization(UL);
//    ul_total_prb_usage_cell.addValue(prb_usage);
}

void EnodeBStatsCollector::add_number_of_active_ue_dl_nongbr_cell()
{
//    int users = mac_->getActiveUeSet(DL);
//    EV << "EnodeBStatsCollector::add_number_of_active_ue_dl_nongbr_cell " << users << endl;
//    number_of_active_ue_dl_nongbr_cell.addValue(users);
}

void EnodeBStatsCollector::add_number_of_active_ue_ul_nongbr_cell()
{
//    int users = mac_->getActiveUeSet(UL);
//    number_of_active_ue_ul_nongbr_cell.addValue(users);
}

void EnodeBStatsCollector::add_dl_nongbr_pdr_cell()
{
    double discard = flowManager_->getDiscardedPkt();
    dl_nongbr_pdr_cell.addValue(discard);
}

void EnodeBStatsCollector::add_ul_nongbr_pdr_cell()
{
    double pdr = 0.0;
    DiscardedPkts pair = {0,0};
    DiscardedPkts temp = {0,0};

    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
    UeStatsCollectorMap::iterator end = ueCollectors_.end();

    for(; it != end ; ++it)
    {
        temp = it->second->getULDiscardedPkt();
        pair.discarded += temp.discarded;
        pair.total += temp.total;
    }

    pdr = ((double)pair.discarded * 1000000)/ pair.total;
    ul_nongbr_pdr_cell.addValue(pdr);
}


//TODO handover management

// for each user save stats

void EnodeBStatsCollector::add_dl_nongbr_pdr_cell_perUser()
{
    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
    UeStatsCollectorMap::iterator end = ueCollectors_.end();
    double discard;
    for(; it != end ; ++it)
    {
        discard = flowManager_->getDiscardedPktPerUe(it->first);
        it->second->add_dl_nongbr_pdr_ue(discard);
    }
}

void EnodeBStatsCollector::add_ul_nongbr_delay_perUser()
{
    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
    UeStatsCollectorMap::iterator end = ueCollectors_.end();
    for(; it != end ; ++it)
    {
        it->second->add_ul_nongbr_delay_ue();
    }
}

void EnodeBStatsCollector::add_dl_nongbr_delay_perUser()
{
    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
    UeStatsCollectorMap::iterator end = ueCollectors_.end();
    double delay;
    for(; it != end ; ++it)
    {
        delay = flowManager_->getDelayStatsPerUe(it->first);
        EV << "EnodeBStatsCollector::add_dl_nongbr_delay_perUser - delay: " << delay << " for node id: " << it->first << endl;
        if(delay != 0)
            it->second->add_dl_nongbr_delay_ue(delay);
    }
}

void EnodeBStatsCollector::add_ul_nongbr_data_volume_ue_perUser()
{
    EV << "EnodeBStatsCollector::add_ul_nongbr_data_volume_ue_perUser" << endl;
    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
    UeStatsCollectorMap::iterator end = ueCollectors_.end();
    unsigned int bytes;
    for(; it != end ; ++it)
    {
//        bytes = pdcp_->getPdcpBytesUlPerUe(it->first);
//        EV << "EnodeBStatsCollector::add_ul_nongbr_data_volume_ue_perUser - received :" << bytes << "B in UL from node id: " << it->first << endl;
//        it->second->add_ul_nongbr_data_volume_ue(bytes);
    }
}

void EnodeBStatsCollector::add_dl_nongbr_data_volume_ue_perUser()
{
    EV << "EnodeBStatsCollector::add_dl_nongbr_data_volume_ue_perUser" << endl;
    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
    UeStatsCollectorMap::iterator end = ueCollectors_.end();
    unsigned int bytes;
    for(; it != end ; ++it)
    {
//        bytes = pdcp_->getPdcpBytesDlPerUe(it->first);
//        EV << "EnodeBStatsCollector::add_dl_nongbr_data_volume_ue_perUser - sent :" << bytes << "B in DL from node id: " << it->first << endl;
//        it->second->add_dl_nongbr_data_volume_ue(bytes);
    }
}

void EnodeBStatsCollector::add_dl_nongbr_throughput_ue_perUser()
{
    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
    UeStatsCollectorMap::iterator end = ueCollectors_.end();
    double throughput;
    for(; it != end ; ++it)
    {
        throughput = flowManager_->getThroughputStatsPerUe(it->first);
        EV << "EnodeBStatsCollector::add_dl_nongbr_throughput_ue_perUser - tput: " << throughput << " for node " << it->first << endl;
        flowManager_->resetThroughputCounterPerUe(it->first);
        if(throughput > 0.0)
            it->second->add_dl_nongbr_throughput_ue(throughput);

    }
}

void EnodeBStatsCollector::add_ul_nongbr_throughput_ue_perUser()
{
    UeStatsCollectorMap::iterator it = ueCollectors_.begin();
    UeStatsCollectorMap::iterator end = ueCollectors_.end();
    double throughput;
    for(; it != end ; ++it)
    {
//        throughput = rlc_->getUeThroughput(it->first);
//        rlc_->resetThroughputStats(it->first);
//        if(throughput > 0.0)
//            it->second->add_ul_nongbr_throughput_ue(throughput);
    }
}

int EnodeBStatsCollector::get_ul_nongbr_pdr_cell()
{
    return ul_nongbr_pdr_cell.getMean();
}


int EnodeBStatsCollector::get_dl_nongbr_pdr_cell()
{
    return dl_nongbr_pdr_cell.getMean();
}

// since GBR rab has not been implemented nongbr = total
int EnodeBStatsCollector::get_dl_total_prb_usage_cell() {
    return dl_total_prb_usage_cell.getMean();
}

int EnodeBStatsCollector::get_ul_total_prb_usage_cell() {
    return ul_total_prb_usage_cell.getMean();
}

int EnodeBStatsCollector::get_dl_nongbr_prb_usage_cell() {
    return dl_total_prb_usage_cell.getMean();
}

int EnodeBStatsCollector::get_ul_nongbr_prb_usage_cell() {
    return ul_total_prb_usage_cell.getMean();
}

int EnodeBStatsCollector::get_number_of_active_ue_dl_nongbr_cell(){
    return number_of_active_ue_dl_nongbr_cell.getMean();
}

int EnodeBStatsCollector::get_number_of_active_ue_ul_nongbr_cell(){
    return number_of_active_ue_ul_nongbr_cell.getMean();
}

const mec::Ecgi& EnodeBStatsCollector::getEcgi() const
{
    return ecgi_;
}

MacCellId EnodeBStatsCollector::getCellId()const
{
    return cellInfo_->getMacCellId();
}
