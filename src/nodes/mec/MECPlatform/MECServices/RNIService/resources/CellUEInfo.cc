//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "CellUEInfo.h"
#include "corenetwork/statsCollector/UeStatsCollector.h"

CellUEInfo::CellUEInfo(){}

CellUEInfo::CellUEInfo(UeStatsCollector* ueCollector, const Ecgi& ecgi):
associateId_(), ecgi_(ecgi)
{
    ueCollector_ = ueCollector;
    associateId_.setAssociateId(ueCollector->getAssociateId());
}

CellUEInfo::CellUEInfo(UeStatsCollector* ueCollector, const mec::Ecgi& ecgi):
associateId_(), ecgi_(ecgi)
{
    ueCollector_ = ueCollector;
    associateId_.setAssociateId(ueCollector->getAssociateId());
}

CellUEInfo::~CellUEInfo(){}

nlohmann::ordered_json CellUEInfo::toJson() const
{
    nlohmann::ordered_json val;

    val["ecgi"] = ecgi_.toJson();
    val["associatedId"] = associateId_.toJson();
    int value;
    value = ueCollector_->get_dl_gbr_delay_ue();
    if(value != -1) val["dl_gbr_delay_ue"] = value;

    value = ueCollector_->get_ul_gbr_delay_ue(); 
    if(value != -1) val["ul_gbr_delay_ue"] = value;
        
    value = ueCollector_->get_dl_nongbr_delay_ue();
    if(value != -1) val["dl_nongbr_delay_ue"] = value;
        
    value = ueCollector_->get_ul_nongbr_delay_ue();
    if(value != -1) val["ul_nongbr_delay_ue"] = value;

    value = ueCollector_->get_dl_gbr_pdr_ue();
    if(value != -1) val["dl_gbr_pdr_ue"] = value;

    value = ueCollector_->get_ul_gbr_pdr_ue();
    if(value != -1) val["ul_gbr_pdr_ue"] = value;

    value = ueCollector_->get_dl_nongbr_pdr_ue();
    if(value != -1) val["dl_nongbr_pdr_ue"] = value;

    value = ueCollector_->get_ul_nongbr_pdr_ue();
    if(value != -1) val["ul_nongbr_pdr_ue"] = value;

    value = ueCollector_->get_dl_nongbr_throughput_ue();
    if(value != -1) val["dl_nongbr_throughput_ue"] = value;

    value = ueCollector_->get_ul_nongbr_throughput_ue();
    if(value != -1) val["ul_nongbr_throughput_ue"] = value;

    value = ueCollector_->get_dl_gbr_throughput_ue();
    if(value != -1) val["dl_gbr_throughput_ue"] = value;

    value = ueCollector_->get_ul_gbr_throughput_ue();
    if(value != -1) val["ul_gbr_throughput_ue"] = value;

    value = ueCollector_->get_dl_nongbr_data_volume_ue();
    if(value != -1) val["dl_nongbr_data_volume_ue"] = value;

    value = ueCollector_->get_ul_nongbr_data_volume_ue();
    if(value != -1) val["ul_nongbr_data_volume_ue"] = value;

    value = ueCollector_->get_dl_gbr_data_volume_ue();
    if(value != -1) val["dl_gbr_data_volume_ue"] = value;

    value = ueCollector_->get_ul_gbr_data_volume_ue();
    if(value != -1) val["ul_gbr_data_volume_ue"] = value;

    return val;
}
