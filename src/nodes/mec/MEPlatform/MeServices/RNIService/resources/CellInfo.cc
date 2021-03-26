#include "../../RNIService/resources/CellInfo.h"

#include "corenetwork/statsCollector/EnodeBStatsCollector.h"

CellInfo::CellInfo(){}

CellInfo::CellInfo(EnodeBStatsCollector* eNodeB){
  collector_ = eNodeB;
  ecgi_.setEcgi(collector_->getEcgi());
//  ueList_ =    eNodeB->getUeListCollectors();
}

CellInfo::~CellInfo(){}


nlohmann::ordered_json CellInfo::toJsonCell() const  {
    nlohmann::ordered_json val;
	val["ecgi"] = ecgi_.toJson();

	int value;
	value = collector_->get_dl_gbr_prb_usage_cell();
	if(value != -1) val["dl_gbr_prb_usage_cell"] = value;
	
	value = collector_->get_ul_gbr_prb_usage_cell();
	if(value != -1) val["ul_gbr_prb_usage_cell"] = value;
	 
	value = collector_->get_dl_nongbr_prb_usage_cell();
	if(value != -1) val["dl_nongbr_prb_usage_cell"] = value;

	value = collector_->get_ul_nongbr_prb_usage_cell();
	if(value != -1) val["ul_nongbr_prb_usage_cell"] = value;

	value = collector_->get_dl_total_prb_usage_cell();
//	if(value != -1)
	    val["dl_total_prb_usage_cell"] = value;

	value = collector_->get_ul_total_prb_usage_cell();
//	if(value != -1)
	    val["ul_total_prb_usage_cell"] = value;

	value = collector_->get_received_dedicated_preambles_cell();
	if(value != -1) val["received_dedicated_preambles_cell"] = value;

	value = collector_->get_received_randomly_selected_preambles_low_range_cell();
	if(value != -1) val["received_randomly_selected_preambles_low_range_cell"] = value;

	value = collector_->get_received_randomly_selected_preambles_high_range_cell();
	if(value != -1) val["received_randomly_selected_preambles_high_range_cell"] = value;
	
	value = collector_->get_number_of_active_ue_dl_gbr_cell();
	if(value != -1) val["number_of_active_ue_dl_gbr_cell"] = value;
	
	value = collector_->get_number_of_active_ue_ul_gbr_cell();
	if(value != -1) val["number_of_active_ue_ul_gbr_cell"] = value;
	
	value = collector_->get_number_of_active_ue_dl_nongbr_cell();
	if(value != -1) val["number_of_active_ue_dl_nongbr_cell"] = value;
	
	value = collector_->get_number_of_active_ue_ul_nongbr_cell();
	if(value != -1) val["number_of_active_ue_ul_nongbr_cell"] = value;
	
	value = collector_->get_dl_gbr_pdr_cell();
	if(value != -1) val["dl_gbr_pdr_cell"] = value;
	
	value = collector_->get_ul_gbr_pdr_cell();
	if(value != -1) val["ul_gbr_pdr_cell"] = value;
	
	value = collector_->get_dl_nongbr_pdr_cell();
	if(value != -1) val["dl_nongbr_pdr_cell"] = value;
	
	value = collector_->get_ul_nongbr_pdr_cell();
	if(value != -1) val["ul_nongbr_pdr_cell"] = value;

	return val;
}

UeStatsCollectorMap* CellInfo::getCollectorMap() const
{
	return collector_->getCollectorMap();
}

Ecgi CellInfo::getEcgi() const
{
	return ecgi_;
}


nlohmann::ordered_json CellInfo::toJson() const {
	nlohmann::ordered_json val = toJsonCell();
	
//	// per UE
//	std::map<ipv4, ueCollector>::const_iterator it = ueList_->getBeginIterator();
//	std::map<ipv4, ueCollector>::const_iterator endIt = ueList_->getBeginIterator();
//
//    nlohmann::ordered_json jsonArray;
//
//	for(; it != endIt; ++it){
//		CellUeInfo ue(&(it->second));
//		jsonArray.push_back(ue.toJson());
//	}
//
//	if(jsonArray.size() > 0)
//	{
//		val["cellUeInfo"] = jsonArray;
//	}
	return val;
}


//nlohmann::ordered_json CellInfo::toJson(std::vector<Ipv4>& uesID) const {
//	nlohmann::ordered_json val = toJsonCell();
//	nlohmann::ordered_json jsonArray;
//	std::vector<Ipv4>&::const_iterator it = uesID.begin();
//	for(; it != uesID.end() ; ++it){
//		if(ueList_.findUeByIpv4(*it) != false){
//			CellUeInfo ue(ueList_.getUserCollectorByIPv4(*it));
//			jsonArray.push_back(ue.toJson());
//		}
//
//	}
//	if(jsonArray.size() > 0)
//	{
//		val["cellUeInfo"] = jsonArray;
//	}
//
//	return val;
//}
