#include "nodes/mec/MEPlatform/MeServices/Resources/SubscriptionBase.h"
#include "corenetwork/lteCellInfo/LteCellInfo.h"

using namespace omnetpp;

SubscriptionBase::SubscriptionBase() {}

SubscriptionBase::SubscriptionBase(unsigned int subId, inet::TcpSocket *socket, const std::string& baseResLocation,  std::vector<cModule*>& eNodeBs) {
    std::vector<cModule*>::iterator it = eNodeBs.begin();
    for(; it != eNodeBs.end() ; ++it){
        LteCellInfo * cellInfo = check_and_cast<LteCellInfo *>((*it)->getSubmodule("cellInfo"));
        eNodeBs_.insert(std::pair<MacCellId, LteCellInfo *>(cellInfo->getMacCellId(), cellInfo));
    }
	subscriptionId_ = subId;
	socket_ = socket;
	baseResLocation_ = baseResLocation;
//	notificationTrigger = nullptr;
}

void SubscriptionBase::addEnodeB(std::vector<cModule*>& eNodeBs) {
    std::vector<cModule*>::iterator it = eNodeBs.begin();
    for(; it != eNodeBs.end() ; ++it){
        LteCellInfo * cellInfo = check_and_cast<LteCellInfo *>((*it)->getSubmodule("cellInfo"));
        eNodeBs_.insert(std::pair<MacCellId, LteCellInfo *>(cellInfo->getMacCellId(), cellInfo));
        EV << "LocationResource::addEnodeB - added eNodeB: " << cellInfo->getMacCellId() << endl;
    }
}

void SubscriptionBase::addEnodeB(cModule* eNodeB) {
    LteCellInfo * cellInfo = check_and_cast<LteCellInfo *>(eNodeB->getSubmodule("cellInfo"));
    eNodeBs_.insert(std::pair<MacCellId, LteCellInfo *>(cellInfo->getMacCellId(), cellInfo));
    EV << "LocationResource::addEnodeB with cellId: "<< cellInfo->getMacCellId() << endl;
    EV << "LocationResource::addEnodeB - added eNodeB: " << cellInfo->getMacCellId() << endl;
}


SubscriptionBase::~SubscriptionBase() {}

//
//nlohmann::ordered_json SubscriptionBase::toJson() const {
////	nlohmann::ordered_json val ;
////	nlohmann::ordered_json SubscriptionBase;
////	nlohmann::ordered_json cellArray;
////	nlohmann::ordered_json ueArray;
////
////	if (timestamp_.isValid())
////	{
//////		timestamp_.setSeconds();
////		val["timestamp"] = timestamp_.toJson();
////	}
////
////	std::map<MacCellId, EnodeBStatsCollector *>::const_iterator it = eNodeBs_.begin();
////	for(; it != eNodeBs_.end() ; ++it){
////	    UeStatsCollectorMap *ueMap = it->second->getCollectorMap();
////        UeStatsCollectorMap::const_iterator uit = ueMap->begin();
////        UeStatsCollectorMap::const_iterator end = ueMap->end();
////        for(; uit != end ; ++uit)
////        {
////            CellUEInfo cellUeInfo = CellUEInfo(uit->second, it->second->getEcgi());
////            ueArray.push_back(cellUeInfo.toJson());
////        }
////		CellInfo cellInfo = CellInfo(it->second);
////		cellArray.push_back(cellInfo.toJson());
////	}
////
////	if(cellArray.size() > 1){
////		val["cellInfo"] = cellArray;
////    }
////	else if(cellArray.size() == 1){
////		val["cellInfo"] = cellArray[0];
////	}
////
////	if(ueArray.size() > 1){
////		val["CellUEInfo"] = ueArray;
////    }
////	else if(ueArray.size() == 1){
////		val["CellUEInfo"] = ueArray[0];
////	}
////
////	SubscriptionBase["SubscriptionBase"] = val;
////	return SubscriptionBase;
//}
//
////
//nlohmann::ordered_json SubscriptionBase::toJsonUe(std::vector<MacNodeId>& uesID) const {
////	nlohmann::ordered_json val ;
////	nlohmann::ordered_json SubscriptionBase;
////	nlohmann::ordered_json ueArray;
////
////	if (timestamp_.isValid())
////	{
//////		timestamp_.setSeconds();
////		val["timestamp"] = timestamp_.toJson();
////	}
////
////
////	std::vector<MacNodeId>::const_iterator uit = uesID.begin();
////	std::map<MacCellId, EnodeBStatsCollector*>::const_iterator eit;
////	bool found = false;
////	for(; uit != uesID.end() ; ++uit){
////	    found = false;
////	    eit = eNodeBs_.begin();
////	    for(; eit != eNodeBs_.end() ; ++eit){
////            if(eit->second->hasUeCollector(*uit))
////            {
////                UeStatsCollector *ueColl = eit->second->getUeCollector(*uit);
////                CellUEInfo cellUeInfo = CellUEInfo(ueColl, eit->second->getEcgi());
////                ueArray.push_back(cellUeInfo.toJson());
////                found = true;
////                break; // next ue id
////            }
////        }
////        if(!found)
////        {
////
////        }
////	}
////
////	if(ueArray.size() > 1){
////        val["CellUEInfo"] = ueArray;
////	}
////    else if(ueArray.size() == 1){
////        val["CellUEInfo"] = ueArray[0];
////    }
////
////	SubscriptionBase["SubscriptionBase"] = val;
////	return SubscriptionBase;
//}
//
////
//nlohmann::ordered_json SubscriptionBase::toJsonCell(std::vector<MacCellId>& cellsID) const
//{
////    nlohmann::ordered_json val ;
////    nlohmann::ordered_json SubscriptionBase;
////    nlohmann::ordered_json cellArray;
////
////        if (timestamp_.isValid())
////        {
////    //      timestamp_.setSeconds();
////            val["timestamp"] = timestamp_.toJson();
////        }
////
////        std::vector<MacCellId>::const_iterator cid =  cellsID.begin();
////        std::map<MacCellId, EnodeBStatsCollector *>::const_iterator it;
////        for(; cid != cellsID.end() ; ++cid){
////            it = eNodeBs_.find(*cid);
////            if(it != eNodeBs_.end()){
////                CellInfo cellInfo = CellInfo(it->second);
////                cellArray.push_back(cellInfo.toJson());
////            }
////        }
////
////        if(cellArray.size() > 1){
////            val["cellInfo"] = cellArray;
////        }
////        else if(cellArray.size() == 1){
////            val["cellInfo"] = cellArray[0];
////        }
////
////        SubscriptionBase["SubscriptionBase"] = val;
////        return SubscriptionBase;
//
//}
//////
//nlohmann::ordered_json SubscriptionBase::toJson(std::vector<MacCellId>& cellsID, std::vector<MacNodeId>& uesID) const
//{
//
//}
//

bool SubscriptionBase::fromJson(const nlohmann::ordered_json& jsonBody)
{

    if(!jsonBody.contains("callbackReference") || jsonBody["callbackReference"].is_array())
    {

        Http::send400Response(socket_); // callbackReference is mandatory and takes exactly 1 att
        return false;
    }

    if(std::string(jsonBody["callbackReference"]).find('/') == -1) //bad uri
    {
        Http::send400Response(socket_); // must be ipv4
        return false;
    }

    callbackReference_ = jsonBody["callbackReference"];

    //chek expiration time
    // TODO add end timer
    if(jsonBody.contains("expiryDeadline") && !jsonBody["expiryDeadline"].is_array())
    {
        expiryTime_.setSeconds(jsonBody["expiryDeadline"]["seconds"]);
        expiryTime_.setNanoSeconds(jsonBody["expiryDeadline"]["nanoSeconds"]);
        expiryTime_.setValid(true);
    }
    else
    {
        expiryTime_.setValid(false);
    }

    links_ = baseResLocation_ +std::to_string(subscriptionId_);

    return true;
}


void SubscriptionBase::set_links(std::string& link)
{
    links_ = link+"sub"+std::to_string(subscriptionId_);
}

std::string SubscriptionBase::getSubscriptionType() const
{
    return subscriptionType_;
}

int SubscriptionBase::getSubscriptionId() const
{
    return subscriptionId_;
}

int SubscriptionBase::getSocketConnId() const
{
    return socket_->getSocketId();
}

