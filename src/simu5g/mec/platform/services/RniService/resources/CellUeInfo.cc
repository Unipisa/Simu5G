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

#include "CellUeInfo.h"
#include "simu5g/corenetwork/statsCollector/UeStatsCollector.h"

namespace simu5g {

CellUeInfo::CellUeInfo() {}

CellUeInfo::CellUeInfo(UeStatsCollector *ueCollector, const Ecgi& ecgi):
    ueCollector_(ueCollector), associateId_(), ecgi_(ecgi)
{
    associateId_.setAssociateId(ueCollector->getAssociateId());
}

CellUeInfo::CellUeInfo(UeStatsCollector *ueCollector, const mec::Ecgi& ecgi):
    ueCollector_(ueCollector), associateId_(), ecgi_(ecgi)
{
    associateId_.setAssociateId(ueCollector->getAssociateId());
}


nlohmann::ordered_json CellUeInfo::toJson() const
{
    nlohmann::ordered_json val;

    val["ecgi"] = ecgi_.toJson();
    val["associatedId"] = associateId_.toJson();
    int value;
    value = ueCollector_->get_dl_gbr_delay_ue();
    if (value != -1) val["dl_gbr_delay_ue"] = value;

    value = ueCollector_->get_ul_gbr_delay_ue();
    if (value != -1) val["ul_gbr_delay_ue"] = value;

    value = ueCollector_->get_dl_nongbr_delay_ue();
    if (value != -1) val["dl_nongbr_delay_ue"] = value;

    value = ueCollector_->get_ul_nongbr_delay_ue();
    if (value != -1) val["ul_nongbr_delay_ue"] = value;

    value = ueCollector_->get_dl_gbr_pdr_ue();
    if (value != -1) val["dl_gbr_pdr_ue"] = value;

    value = ueCollector_->get_ul_gbr_pdr_ue();
    if (value != -1) val["ul_gbr_pdr_ue"] = value;

    value = ueCollector_->get_dl_nongbr_pdr_ue();
    if (value != -1) val["dl_nongbr_pdr_ue"] = value;

    value = ueCollector_->get_ul_nongbr_pdr_ue();
    if (value != -1) val["ul_nongbr_pdr_ue"] = value;

    value = ueCollector_->get_dl_nongbr_throughput_ue();
    if (value != -1) val["dl_nongbr_throughput_ue"] = value;

    value = ueCollector_->get_ul_nongbr_throughput_ue();
    if (value != -1) val["ul_nongbr_throughput_ue"] = value;

    value = ueCollector_->get_dl_gbr_throughput_ue();
    if (value != -1) val["dl_gbr_throughput_ue"] = value;

    value = ueCollector_->get_ul_gbr_throughput_ue();
    if (value != -1) val["ul_gbr_throughput_ue"] = value;

    value = ueCollector_->get_dl_nongbr_data_volume_ue();
    if (value != -1) val["dl_nongbr_data_volume_ue"] = value;

    value = ueCollector_->get_ul_nongbr_data_volume_ue();
    if (value != -1) val["ul_nongbr_data_volume_ue"] = value;

    value = ueCollector_->get_dl_gbr_data_volume_ue();
    if (value != -1) val["dl_gbr_data_volume_ue"] = value;

    value = ueCollector_->get_ul_gbr_data_volume_ue();
    if (value != -1) val["ul_gbr_data_volume_ue"] = value;

    return val;
}

} //namespace

