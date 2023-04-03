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

#include "corenetwork/statsCollector/BaseStationStatsCollector.h"
#include "corenetwork/statsCollector/UeStatsCollector.h"
#include "stack/packetFlowManager/PacketFlowManagerEnb.h"
#include "stack/rlc/um/LteRlcUm.h"
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"
#include "stack/mac/layer/LteMacEnb.h"
#include <string>

Define_Module(BaseStationStatsCollector);
BaseStationStatsCollector::BaseStationStatsCollector()
{
    pdcp_ = nullptr;
    rlc_ = nullptr;
    mac_ = nullptr;
    packetFlowManager_ = nullptr;

    prbUsage_ = nullptr;
    activeUsers_ = nullptr;;
    discardRate_ = nullptr;;
    packetDelay_ = nullptr;;
    pdcpBytes_ = nullptr;;
    tPut_ = nullptr;
}


BaseStationStatsCollector::~BaseStationStatsCollector()
{
    cancelAndDelete(pdcpBytes_);
    cancelAndDelete(prbUsage_);
    cancelAndDelete(discardRate_);
    cancelAndDelete(activeUsers_);
    cancelAndDelete(packetDelay_);
    cancelAndDelete(tPut_);

}

void BaseStationStatsCollector::initialize(int stage){

    if(stage == inet::INITSTAGE_LOCAL)
    {
        EV << collectorType_ << "::initialize stage: "<< stage << endl;
        collectorType_ = par("collectorType").stringValue();
        std::string nodeType =  getAncestorPar("nodeType").stringValue();
        nodeType_ = nodeType.compare("ENODEB") == 0? ENODEB: GNODEB;
        EV << collectorType_ << "::initialize node type: "<< nodeType << endl;
    }
    else if (stage == inet::INITSTAGE_APPLICATION_LAYER)
    {
        EV << collectorType_ << "::initialize stage: "<< stage << endl;

        ecgi_.plmn.mcc = getAncestorPar("mcc").stdstringValue();
        ecgi_.plmn.mnc = getAncestorPar("mnc").stdstringValue();

        mac_ = check_and_cast<LteMacEnb *>(getParentModule()->getSubmodule("cellularNic")->getSubmodule("mac"));
        pdcp_ = check_and_cast<LtePdcpRrcEnb *>(getParentModule()->getSubmodule("cellularNic")->getSubmodule("pdcpRrc"));

        cModule *rlc = getParentModule()->getSubmodule("cellularNic")->getSubmodule("rlc");
        if(rlc->findSubmodule("um") != -1)
        {
            rlc_ = check_and_cast<LteRlcUm *>(getParentModule()->getSubmodule("cellularNic")->getSubmodule("rlc")->getSubmodule("um"));
        }
        else
        {
            throw cRuntimeError("%s::initialize - EnodeB statistic collector only works with RLC in UM mode", collectorType_.c_str() );
        }
        
        if(getParentModule()->getSubmodule("cellularNic")->findSubmodule("packetFlowManager") != -1)
        {
            EV << collectorType_ << "::initialize - packetFlowManager reference" << endl;
            packetFlowManager_ = check_and_cast<PacketFlowManagerEnb *>(getParentModule()->getSubmodule("cellularNic")->getSubmodule("packetFlowManager"));
        }

        cellInfo_ = check_and_cast<CellInfo *>(getParentModule()->getSubmodule("cellInfo"));
        ecgi_.cellId = cellInfo_->getMacCellId(); // at least stage 2

        dl_total_prb_usage_cell.init("dl_total_prb_usage_cell", par("prbUsagePeriods"), par("movingAverage"));
        ul_total_prb_usage_cell.init("ul_total_prb_usage_cell", par("prbUsagePeriods"), par("movingAverage"));

        number_of_active_ue_dl_nongbr_cell.init("number_of_active_ue_dl_nongbr_cell", par("activeUserPeriods"), false);
        number_of_active_ue_ul_nongbr_cell.init("number_of_active_ue_ul_nongbr_cell", par("activeUserPeriods"), false);

        dl_nongbr_pdr_cell.init("dl_nongbr_pdr_cell", par("discardRatePeriods"), false);
        ul_nongbr_pdr_cell.init("ul_nongbr_pdr_cell", par("discardRatePeriods"), false);

        // setup timers
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
        if(packetFlowManager_ != nullptr)
        {
            scheduleAt(NOW + dataVolumePeriod_, pdcpBytes_);
            scheduleAt(NOW + discardRatePeriod_, discardRate_);
            scheduleAt(NOW + delayPacketPeriod_, packetDelay_);
            scheduleAt(NOW + tPutPeriod_,tPut_);
        }
    }
}


void BaseStationStatsCollector::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage())
    {
        EV << collectorType_ << "::handleMessage - get " << msg->getName() << "statistics" << endl;

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
            add_ul_nongbr_pdr_cell_perUser();

            //reset counters
            packetFlowManager_->resetDiscardCounter();
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
    {
        EV << collectorType_ << "::handleMessage - it is not a self message" << endl;
        delete msg;
    }
}


void BaseStationStatsCollector::resetDiscardCounterPerUe()
{
    EV << collectorType_ << "::resetDiscardCounterPerUe " << endl;
    for(auto const &ue : ueCollectors_)
    {
        packetFlowManager_->resetDiscardCounterPerUe(ue.first);
    }
}

void BaseStationStatsCollector::resetDelayCounterPerUe()
{
    EV << collectorType_ << "::resetDelayCounterPerUe " << endl;
    for(auto const &ue : ueCollectors_)
    {
        packetFlowManager_->resetDelayCounterPerUe(ue.first);
        ue.second->resetDelayCounter();
    }
}

void BaseStationStatsCollector::resetThroughputCountersPerUe()
{
    EV << collectorType_ << "::resetThroughputCountersPerUe " << endl;
    for(auto const &ue : ueCollectors_)
    {
        packetFlowManager_->resetThroughputCounterPerUe(ue.first);
    }
}

void BaseStationStatsCollector::resetBytesCountersPerUe()
{
    EV << collectorType_ << "::resetBytesCountersPerUe " << endl;
    for(auto const &ue : ueCollectors_)
    {
       packetFlowManager_->resetDataVolume(ue.first);
    }
}


// BaseStationStatsCollector management methods
void BaseStationStatsCollector::addUeCollector(MacNodeId id, UeStatsCollector* ueCollector)
{
    if(ueCollectors_.find(id) == ueCollectors_.end())
    {
        ueCollectors_.insert(std::pair<MacNodeId,UeStatsCollector *>(id, ueCollector));
    }
    else
    {
        throw cRuntimeError("%s::addUeCollector - UeStatsCollector already present for UE nodeid[%d]", collectorType_.c_str(), id);
    }
}

void BaseStationStatsCollector::removeUeCollector(MacNodeId id)
{
    std::map<MacNodeId, UeStatsCollector*>::iterator it = ueCollectors_.find(id);
    if(it != ueCollectors_.end())
    {
        ueCollectors_.erase(it);
        EV << "BaseStationStatsCollector::removeUeCollector - removing UE pfm stats for UE with id["<<id<<"]"<< endl;
        packetFlowManager_->deleteUe(id);
    }
    else
    {
        throw cRuntimeError("%s::removeUeCollector - UeStatsCollector not present for UE nodeid[%d]", collectorType_.c_str(), id);
    }
}

UeStatsCollector* BaseStationStatsCollector::getUeCollector(MacNodeId id)
{
    std::map<MacNodeId, UeStatsCollector*>::iterator it = ueCollectors_.find(id);
    if(it != ueCollectors_.end())
    {
       return it->second;
    }
    else
    {
       throw cRuntimeError("%s::removeUeCollector - UeStatsCollector not present for UE nodeid[%d]", collectorType_.c_str(), id);
    }
}

UeStatsCollectorMap* BaseStationStatsCollector::getCollectorMap()
{
    return &ueCollectors_;
}

bool BaseStationStatsCollector::hasUeCollector(MacNodeId id)
{
    return (ueCollectors_.find(id) != ueCollectors_.end()) ? true : false;
}



void BaseStationStatsCollector::add_dl_total_prb_usage_cell()
{
    double prb_usage = mac_->getUtilization(DL);
    EV << collectorType_ << "::add_dl_total_prb_usage_cell " << prb_usage << "%"<< endl;
    dl_total_prb_usage_cell.addValue((int)prb_usage);
}

void BaseStationStatsCollector::add_ul_total_prb_usage_cell()
{
    double prb_usage = mac_->getUtilization(UL);
    EV << collectorType_ << "::add_ul_total_prb_usage_cell " << prb_usage << "%"<< endl;
    ul_total_prb_usage_cell.addValue((int)prb_usage);
}

void BaseStationStatsCollector::add_number_of_active_ue_dl_nongbr_cell()
{
    int users = mac_->getActiveUesNumber(DL);
    EV << collectorType_ << "::add_number_of_active_ue_dl_nongbr_cell " << users << endl;
    number_of_active_ue_dl_nongbr_cell.addValue(users);
}

void BaseStationStatsCollector::add_number_of_active_ue_ul_nongbr_cell()
{
    int users = mac_->getActiveUesNumber(UL);
    EV << collectorType_ << "::add_number_of_active_ue_ul_nongbr_cell " << users << endl;
    number_of_active_ue_ul_nongbr_cell.addValue(users);
}

void BaseStationStatsCollector::add_dl_nongbr_pdr_cell()
{
    double discard = packetFlowManager_->getDiscardedPkt();
    dl_nongbr_pdr_cell.addValue((int)discard);
}

void BaseStationStatsCollector::add_ul_nongbr_pdr_cell()
{
    double pdr = 0.0;
    DiscardedPkts pair = {0,0};
    DiscardedPkts temp = {0,0};

    for(auto const &ue : ueCollectors_)
    {
        temp = ue.second->getULDiscardedPkt();
        pair.discarded += temp.discarded;
        pair.total += temp.total;
    }

    if (pair.total == 0)
        pdr = 0.0;
    else
        pdr = ((double)pair.discarded * 1000000)/ pair.total;
    ul_nongbr_pdr_cell.addValue((int)pdr);
}


//TODO handover management

// for each user save stats

void BaseStationStatsCollector::add_dl_nongbr_pdr_cell_perUser()
{
    EV << collectorType_ << "add_dl_nongbr_pdr_cell_perUser()" << endl;;
    double discard;
    for(auto const &ue : ueCollectors_)
    {
        discard = packetFlowManager_->getDiscardedPktPerUe(ue.first);
        ue.second->add_dl_nongbr_pdr_ue((int)discard);
    }
}

void BaseStationStatsCollector::add_ul_nongbr_pdr_cell_perUser()
{
    EV << collectorType_ << "add_ul_nongbr_pdr_cell_perUser()" << endl;
    for(auto const &ue : ueCollectors_)
    {
        ue.second->add_ul_nongbr_pdr_ue();
    }
}

void BaseStationStatsCollector::add_ul_nongbr_delay_perUser()
{
    EV << collectorType_ << "add_ul_nongbr_delay_perUser()" << endl;
    for(auto const &ue : ueCollectors_)
    {
        ue.second->add_ul_nongbr_delay_ue();
    }
}

void BaseStationStatsCollector::add_dl_nongbr_delay_perUser()
{
    EV << collectorType_ << "add_dl_nongbr_delay_perUser()" << endl;;
    double delay;
    for(auto const &ue : ueCollectors_)
    {
        delay = packetFlowManager_->getDelayStatsPerUe(ue.first);
        EV << collectorType_ << "::add_dl_nongbr_delay_perUser - delay: " << delay << " for node id: " << ue.first << endl;
        if(delay != 0)
        {
            ue.second->add_dl_nongbr_delay_ue((int)delay);
        }
    }
}

void BaseStationStatsCollector::add_ul_nongbr_data_volume_ue_perUser()
{
    EV << collectorType_ << "::add_ul_nongbr_data_volume_ue_perUser" << endl;
    unsigned int bytes;
    for(auto const &ue : ueCollectors_)
    {
        bytes = packetFlowManager_->getDataVolume(ue.first, UL);
        EV << collectorType_ << "::add_ul_nongbr_data_volume_ue_perUser - received :" << bytes << "B in UL from node id: " << ue.first << endl;
        ue.second->add_ul_nongbr_data_volume_ue(bytes);
    }
}

void BaseStationStatsCollector::add_dl_nongbr_data_volume_ue_perUser()
{
    EV << collectorType_ << "::add_dl_nongbr_data_volume_ue_perUser" << endl;
       unsigned int bytes;
       for(auto const &ue : ueCollectors_)
       {
           bytes = packetFlowManager_->getDataVolume(ue.first, DL);
           EV << collectorType_ << "::add_dl_nongbr_data_volume_ue_perUser - sent :" << bytes << "B in DL to node id: " << ue.first << endl;
           ue.second->add_dl_nongbr_data_volume_ue(bytes);
       }
}

void BaseStationStatsCollector::add_dl_nongbr_throughput_ue_perUser()
{
    EV << collectorType_ << "::add_dl_nongbr_throughput_ue_perUser" << endl;
    double throughput;
    for(auto const &ue : ueCollectors_)
    {
        throughput = packetFlowManager_->getThroughputStatsPerUe(ue.first);
        EV << collectorType_ << "::add_dl_nongbr_throughput_ue_perUser - tput: " << throughput << " for node " << ue.first << endl;
        packetFlowManager_->resetThroughputCounterPerUe(ue.first);
        if(throughput > 0.0)
            ue.second->add_dl_nongbr_throughput_ue((int)throughput);
    }
}

void BaseStationStatsCollector::add_ul_nongbr_throughput_ue_perUser()
{

    EV << collectorType_ << "::add_ul_nongbr_throughput_ue_perUser" << endl;
    double throughput;
    for(auto const &ue : ueCollectors_)
    {
        throughput = rlc_->getUeThroughput(ue.first);
        EV << collectorType_ << "::add_ul_nongbr_throughput_ue_perUser - tput: " << throughput << " for node " << ue.first << endl;
        rlc_->resetThroughputStats(ue.first);
        if(throughput > 0.0)
            ue.second->add_ul_nongbr_throughput_ue((int)throughput);
    }
}


/*
 * Getters for RNI service module
 */

int BaseStationStatsCollector::get_ul_nongbr_pdr_cell()
{
    return ul_nongbr_pdr_cell.getMean();
}

int BaseStationStatsCollector::get_dl_nongbr_pdr_cell()
{
    return dl_nongbr_pdr_cell.getMean();
}

// since GBR rab has not been implemented nongbr = total
int BaseStationStatsCollector::get_dl_total_prb_usage_cell() {
    return dl_total_prb_usage_cell.getMean();
}

int BaseStationStatsCollector::get_ul_total_prb_usage_cell() {
    return ul_total_prb_usage_cell.getMean();
}

int BaseStationStatsCollector::get_dl_nongbr_prb_usage_cell() {
    return dl_total_prb_usage_cell.getMean();
}

int BaseStationStatsCollector::get_ul_nongbr_prb_usage_cell() {
    return ul_total_prb_usage_cell.getMean();
}

int BaseStationStatsCollector::get_number_of_active_ue_dl_nongbr_cell(){
    return number_of_active_ue_dl_nongbr_cell.getMean();
}

int BaseStationStatsCollector::get_number_of_active_ue_ul_nongbr_cell(){
    return number_of_active_ue_ul_nongbr_cell.getMean();
}

const mec::Ecgi& BaseStationStatsCollector::getEcgi() const
{
    return ecgi_;
}

MacCellId BaseStationStatsCollector::getCellId()const
{
    return cellInfo_->getMacCellId();
}

void BaseStationStatsCollector::resetStats(MacNodeId nodeId)
{
    auto ue = ueCollectors_.find(nodeId);
    if(ue != ueCollectors_.end())
        ue->second->resetStats();
}

