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

#include "RNICellInfo.h"
#include "corenetwork/statsCollector/BaseStationStatsCollector.h"

namespace simu5g {

RNICellInfo::RNICellInfo()
{
}

RNICellInfo::RNICellInfo(BaseStationStatsCollector *eNodeB) : collector_(eNodeB)
{
    ecgi_.setEcgi(collector_->getEcgi());
}


nlohmann::ordered_json RNICellInfo::toJsonCell() const
{
    nlohmann::ordered_json val;
    val["ecgi"] = ecgi_.toJson();

    int value;
    value = collector_->get_dl_gbr_prb_usage_cell();
    if (value != -1) val["dl_gbr_prb_usage_cell"] = value;

    value = collector_->get_ul_gbr_prb_usage_cell();
    if (value != -1) val["ul_gbr_prb_usage_cell"] = value;

    value = collector_->get_dl_nongbr_prb_usage_cell();
    if (value != -1) val["dl_nongbr_prb_usage_cell"] = value;

    value = collector_->get_ul_nongbr_prb_usage_cell();
    if (value != -1) val["ul_nongbr_prb_usage_cell"] = value;

    value = collector_->get_dl_total_prb_usage_cell();
    val["dl_total_prb_usage_cell"] = value;

    value = collector_->get_ul_total_prb_usage_cell();
    val["ul_total_prb_usage_cell"] = value;

    value = collector_->get_received_dedicated_preambles_cell();
    if (value != -1) val["received_dedicated_preambles_cell"] = value;

    value = collector_->get_received_randomly_selected_preambles_low_range_cell();
    if (value != -1) val["received_randomly_selected_preambles_low_range_cell"] = value;

    value = collector_->get_received_randomly_selected_preambles_high_range_cell();
    if (value != -1) val["received_randomly_selected_preambles_high_range_cell"] = value;

    value = collector_->get_number_of_active_ue_dl_gbr_cell();
    if (value != -1) val["number_of_active_ue_dl_gbr_cell"] = value;

    value = collector_->get_number_of_active_ue_ul_gbr_cell();
    if (value != -1) val["number_of_active_ue_ul_gbr_cell"] = value;

    value = collector_->get_number_of_active_ue_dl_nongbr_cell();
    if (value != -1) val["number_of_active_ue_dl_nongbr_cell"] = value;

    value = collector_->get_number_of_active_ue_ul_nongbr_cell();
    if (value != -1) val["number_of_active_ue_ul_nongbr_cell"] = value;

    value = collector_->get_dl_gbr_pdr_cell();
    if (value != -1) val["dl_gbr_pdr_cell"] = value;

    value = collector_->get_ul_gbr_pdr_cell();
    if (value != -1) val["ul_gbr_pdr_cell"] = value;

    value = collector_->get_dl_nongbr_pdr_cell();
    if (value != -1) val["dl_nongbr_pdr_cell"] = value;

    value = collector_->get_ul_nongbr_pdr_cell();
    if (value != -1) val["ul_nongbr_pdr_cell"] = value;

    return val;
}

UeStatsCollectorMap *RNICellInfo::getCollectorMap() const
{
    return collector_->getCollectorMap();
}

Ecgi RNICellInfo::getEcgi() const
{
    return ecgi_;
}

nlohmann::ordered_json RNICellInfo::toJson() const
{
    nlohmann::ordered_json val = toJsonCell();
    return val;
}

} //namespace

