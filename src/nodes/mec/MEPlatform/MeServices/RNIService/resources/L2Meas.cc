#include "nodes/mec/MEPlatform/MeServices/RNIService/resources/L2Meas.h"
#include "corenetwork/statsCollector/UeStatsCollector.h"
#include "corenetwork/statsCollector/EnodeBStatsCollector.h"

#include "CellUEInfo.h"

L2Meas::L2Meas() {}

L2Meas::L2Meas(std::vector<cModule*>& eNodeBs) {
	std::vector<cModule*>::iterator it = eNodeBs.begin();
	for(; it != eNodeBs.end() ; ++it){
		EnodeBStatsCollector * collector = check_and_cast<EnodeBStatsCollector *>((*it)->getSubmodule("collector"));
		eNodeBs_.insert(std::pair<MacCellId, EnodeBStatsCollector *>(collector->getCellId(), collector));
	}
}

void L2Meas::addEnodeB(std::vector<cModule*>& eNodeBs) {
    std::vector<cModule*>::iterator it = eNodeBs.begin();
        for(; it != eNodeBs.end() ; ++it){
			EnodeBStatsCollector * collector = check_and_cast<EnodeBStatsCollector *>((*it)->getSubmodule("collector"));
            eNodeBs_.insert(std::pair<MacCellId, EnodeBStatsCollector*>(collector->getCellId(), collector));
        }
}

void L2Meas::addEnodeB(cModule* eNodeB) {
    EnodeBStatsCollector * collector = check_and_cast<EnodeBStatsCollector *>(eNodeB->getSubmodule("collector"));
	eNodeBs_.insert(std::pair<MacCellId, EnodeBStatsCollector *>(collector->getCellId(), collector));
}


L2Meas::~L2Meas() {}


nlohmann::ordered_json L2Meas::toJson() const {
	nlohmann::ordered_json val ;
	nlohmann::ordered_json l2Meas;
	nlohmann::ordered_json cellArray;
	nlohmann::ordered_json ueArray;

	if (timestamp_.isValid())
	{
//		timestamp_.setSeconds();
		val["timestamp"] = timestamp_.toJson();
	}

	std::map<MacCellId, EnodeBStatsCollector *>::const_iterator it = eNodeBs_.begin();
	for(; it != eNodeBs_.end() ; ++it){
	    UeStatsCollectorMap *ueMap = it->second->getCollectorMap();
        UeStatsCollectorMap::const_iterator uit = ueMap->begin();
        UeStatsCollectorMap::const_iterator end = ueMap->end();
        for(; uit != end ; ++uit)
        {
            CellUEInfo cellUeInfo = CellUEInfo(uit->second, it->second->getEcgi());
            ueArray.push_back(cellUeInfo.toJson());
        }
		CellInfo cellInfo = CellInfo(it->second);
		cellArray.push_back(cellInfo.toJson());
	}

	if(cellArray.size() > 1){
		val["cellInfo"] = cellArray;
    }
	else if(cellArray.size() == 1){
		val["cellInfo"] = cellArray[0];
	}

	if(ueArray.size() > 1){
		val["cellUEInfo"] = ueArray;
    }
	else if(ueArray.size() == 1){
		val["cellUEInfo"] = ueArray[0];
	}
	
	l2Meas["L2Meas"] = val;
	return l2Meas;
}

//
nlohmann::ordered_json L2Meas::toJsonUe(std::vector<MacNodeId>& uesID) const {
	nlohmann::ordered_json val ;
	nlohmann::ordered_json l2Meas;
	nlohmann::ordered_json ueArray;

	if (timestamp_.isValid())
	{
//		timestamp_.setSeconds();
		val["timestamp"] = timestamp_.toJson();
	}


	std::vector<MacNodeId>::const_iterator uit = uesID.begin();
	std::map<MacCellId, EnodeBStatsCollector*>::const_iterator eit;
	bool found = false;
	for(; uit != uesID.end() ; ++uit){
	    found = false;
	    eit = eNodeBs_.begin();
	    for(; eit != eNodeBs_.end() ; ++eit){
            if(eit->second->hasUeCollector(*uit))
            {
                UeStatsCollector *ueColl = eit->second->getUeCollector(*uit);
                CellUEInfo cellUeInfo = CellUEInfo(ueColl, eit->second->getEcgi());
                ueArray.push_back(cellUeInfo.toJson());
                found = true;
                break; // next ue id
            }
        }
        if(!found)
        {

        }
	}

	if(ueArray.size() > 1){
        val["CellUEInfo"] = ueArray;
	}
    else if(ueArray.size() == 1){
        val["CellUEInfo"] = ueArray[0];
    }

	l2Meas["L2Meas"] = val;
	return l2Meas;
}


//
nlohmann::ordered_json L2Meas::toJsonCell(std::vector<MacCellId>& cellsID) const
{
    nlohmann::ordered_json val ;
    nlohmann::ordered_json l2Meas;
    nlohmann::ordered_json cellArray;

        if (timestamp_.isValid())
        {
    //      timestamp_.setSeconds();
            val["timestamp"] = timestamp_.toJson();
        }

        std::vector<MacCellId>::const_iterator cid =  cellsID.begin();
        std::map<MacCellId, EnodeBStatsCollector *>::const_iterator it;
        for(; cid != cellsID.end() ; ++cid){
            it = eNodeBs_.find(*cid);
            if(it != eNodeBs_.end()){
                CellInfo cellInfo = CellInfo(it->second);
                cellArray.push_back(cellInfo.toJson());
            }
        }

        if(cellArray.size() > 1){
            val["cellInfo"] = cellArray;
        }
        else if(cellArray.size() == 1){
            val["cellInfo"] = cellArray[0];
        }

        l2Meas["L2Meas"] = val;
        return l2Meas;

}
////
nlohmann::ordered_json L2Meas::toJson(std::vector<MacCellId>& cellsID, std::vector<MacNodeId>& uesID) const
{
	nlohmann::ordered_json val ;
    nlohmann::ordered_json l2Meas;
	val["cellInfo"] = toJsonCell(cellsID)["L2Meas"]["cellInfo"];
	val["CellUEInfo"] = toJsonUe(uesID)["L2Meas"]["CellUEInfo"];
	l2Meas["L2Meas"] = val;
	return l2Meas;

	
}
