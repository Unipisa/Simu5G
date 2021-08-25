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

#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/L2Meas.h"
#include "corenetwork/statsCollector/UeStatsCollector.h"
#include "corenetwork/statsCollector/BaseStationStatsCollector.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/RNICellInfo.h"
#include "common/binder/Binder.h"
#include "CellUEInfo.h"

L2Meas::L2Meas() {
    binder_ = getBinder();
}

L2Meas::L2Meas(std::set<omnetpp::cModule*>& eNodeBs) {
	std::set<omnetpp::cModule*>::iterator it = eNodeBs.begin();
	for(; it != eNodeBs.end() ; ++it){
		BaseStationStatsCollector * collector = check_and_cast<BaseStationStatsCollector *>((*it)->getSubmodule("collector"));
		eNodeBs_.insert(std::pair<MacCellId, BaseStationStatsCollector *>(collector->getCellId(), collector));
	}
}

void L2Meas::addEnodeB(std::set<omnetpp::cModule*>& eNodeBs) {
    std::set<omnetpp::cModule*>::iterator it = eNodeBs.begin();
        for(; it != eNodeBs.end() ; ++it){
			BaseStationStatsCollector * collector = check_and_cast<BaseStationStatsCollector *>((*it)->getSubmodule("collector"));
            eNodeBs_.insert(std::pair<MacCellId, BaseStationStatsCollector*>(collector->getCellId(), collector));
        }
}

void L2Meas::addEnodeB(omnetpp::cModule* eNodeB) {
    BaseStationStatsCollector * collector = check_and_cast<BaseStationStatsCollector *>(eNodeB->getSubmodule("collector"));
	eNodeBs_.insert(std::pair<MacCellId, BaseStationStatsCollector *>(collector->getCellId(), collector));
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

	std::map<MacCellId, BaseStationStatsCollector *>::const_iterator it = eNodeBs_.begin();
	for(; it != eNodeBs_.end() ; ++it){
	    UeStatsCollectorMap *ueMap = it->second->getCollectorMap();
        UeStatsCollectorMap::const_iterator uit = ueMap->begin();
        UeStatsCollectorMap::const_iterator end = ueMap->end();
        for(; uit != end ; ++uit)
        {
            CellUEInfo cellUeInfo = CellUEInfo(uit->second, it->second->getEcgi());
            ueArray.push_back(cellUeInfo.toJson());
        }
		RNICellInfo cellInfo = RNICellInfo(it->second);
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
	
	//  l2Meas["L2Meas"] = val;
    //  return l2Meas;
        return val;}

//
nlohmann::ordered_json L2Meas::toJsonUe(std::vector<inet::Ipv4Address>& uesID) const {
	nlohmann::ordered_json val ;
	nlohmann::ordered_json l2Meas;
	nlohmann::ordered_json ueArray;

	if (timestamp_.isValid())
	{
//		timestamp_.setSeconds();
		val["timestamp"] = timestamp_.toJson();
	}

	std::map<MacCellId, BaseStationStatsCollector*>::const_iterator eit;
	bool found = false;
	for(auto ipAddress: uesID){

	    /*
	     * an UE can be connected to both eNB and gNB (at the same time)
	     * I decided to report both the structures.
	     *
	     *
	     */
        MacNodeId lteNodeId = binder_->getMacNodeId(ipAddress);
        MacNodeId nrNodeId = binder_->getNrMacNodeId(ipAddress);
        std::vector<MacNodeId> nodeIds;

        // TODO REORGANIZE CODE!!

        if(lteNodeId == 0 && nrNodeId == 0)
        {
            std::string notFound = "Address: " + ipAddress.str() + " Not found.";
            ueArray.push_back(notFound);
            break;
        }
        else if(lteNodeId != 0 && lteNodeId == nrNodeId) //only nr
        {
            found = false;
            eit = eNodeBs_.begin();
            for(; eit != eNodeBs_.end() ; ++eit){
                if(eit->second->getCellNodeType() == ENODEB) // enodeb does not have nrNodeId
                    continue;
               if(eit->second->hasUeCollector(nrNodeId))
               {
                   UeStatsCollector *ueColl = eit->second->getUeCollector(nrNodeId);
                   CellUEInfo cellUeInfo = CellUEInfo(ueColl, eit->second->getEcgi());
                   ueArray.push_back(cellUeInfo.toJson());
                   found = true;
                   break; // next ue id
               }
            }
            if(!found)
            {
               // ETSI sandBox does not return anything in case the ip is not valid
               std::string notFound = "Address: " + ipAddress.str() + " Not found.";
               ueArray.push_back(notFound);
            }
        }
        else if(lteNodeId != 0 && nrNodeId == 0) //only lte
        {
            found = false;
            eit = eNodeBs_.begin();
            for(; eit != eNodeBs_.end() ; ++eit){
                if(eit->second->getCellNodeType() == GNODEB) // gnodeb does not have lteNodeId
                    continue;
               if(eit->second->hasUeCollector(lteNodeId))
               {
                   UeStatsCollector *ueColl = eit->second->getUeCollector(lteNodeId);
                   CellUEInfo cellUeInfo = CellUEInfo(ueColl, eit->second->getEcgi());
                   ueArray.push_back(cellUeInfo.toJson());
                   found = true;
                   break; // next ue id
               }
            }
            if(!found)
            {
               std::string notFound = "Address: " + ipAddress.str() + " Not found.";
               ueArray.push_back(notFound);
            }
        }
        else if(lteNodeId != nrNodeId && nrNodeId != 0 && lteNodeId != 0) // both lte and nr
        {
            found = false;
            eit = eNodeBs_.begin();
            for(; eit != eNodeBs_.end() ; ++eit){
              if(eit->second->hasUeCollector(lteNodeId))
              {
                  UeStatsCollector *ueColl = eit->second->getUeCollector(lteNodeId);
                  CellUEInfo cellUeInfo = CellUEInfo(ueColl, eit->second->getEcgi());
                  ueArray.push_back(cellUeInfo.toJson());
                  found = true;
                  break; // next ue id
              }
            }

            found = false;
            eit = eNodeBs_.begin();
            for(; eit != eNodeBs_.end() ; ++eit){
              if(eit->second->hasUeCollector(nrNodeId))
              {
                  UeStatsCollector *ueColl = eit->second->getUeCollector(nrNodeId);
                  CellUEInfo cellUeInfo = CellUEInfo(ueColl, eit->second->getEcgi());
                  ueArray.push_back(cellUeInfo.toJson());
                  found = true;
                  break; // next ue id
              }
            }
            if(!found)
            {
              std::string notFound = "Address: " + ipAddress.str() + " Not found.";
              ueArray.push_back(notFound);
            }

        }


	}

	if(ueArray.size() > 1){
        val["cellUEInfo"] = ueArray;
	}
    else if(ueArray.size() == 1){
        val["cellUEInfo"] = ueArray[0];
    }

//  l2Meas["L2Meas"] = val;
//	return l2Meas;
	return val;
}

//
nlohmann::ordered_json L2Meas::toJsonCell(std::vector<MacCellId>& cellsID) const
{
    nlohmann::ordered_json val ;
    nlohmann::ordered_json l2Meas;
    nlohmann::ordered_json cellArray;
    nlohmann::ordered_json ueArray;


        if (timestamp_.isValid())
        {
    //      timestamp_.setSeconds();
            val["timestamp"] = timestamp_.toJson();
        }

        std::vector<MacCellId>::const_iterator cid =  cellsID.begin();
        std::map<MacCellId, BaseStationStatsCollector *>::const_iterator it;
        for(; cid != cellsID.end() ; ++cid){
            it = eNodeBs_.find(*cid);
            if(it != eNodeBs_.end()){
                RNICellInfo cellInfo = RNICellInfo(it->second);
                cellArray.push_back(cellInfo.toJson());

                UeStatsCollectorMap *ueMap = it->second->getCollectorMap();
                UeStatsCollectorMap::const_iterator uit = ueMap->begin();
                UeStatsCollectorMap::const_iterator end = ueMap->end();
                for(; uit != end ; ++uit)
                {
                    CellUEInfo cellUeInfo = CellUEInfo(uit->second, it->second->getEcgi());
                    ueArray.push_back(cellUeInfo.toJson());
                }
            }
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
        //  l2Meas["L2Meas"] = val;
        //  return l2Meas;
            return val;
}
////
nlohmann::ordered_json L2Meas::toJson(std::vector<MacCellId>& cellsID, std::vector<inet::Ipv4Address>& uesID) const
{
	nlohmann::ordered_json val ;
    nlohmann::ordered_json l2Meas;
	val["cellInfo"] = toJsonCell(cellsID)["L2Meas"]["cellInfo"];
	val["cellUEInfo"] = toJsonUe(uesID)["L2Meas"]["cellUEInfo"];
	l2Meas["L2Meas"] = val;
	return l2Meas;

	
}
