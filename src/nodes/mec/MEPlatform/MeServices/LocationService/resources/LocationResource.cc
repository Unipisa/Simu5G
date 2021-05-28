#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/LocationResource.h"


#include "corenetwork/lteCellInfo/LteCellInfo.h"
#include "inet/mobility/base/MovingMobilityBase.h"
#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/LocationApiDefs.h"

#include "nodes/binder/LteBinder.h"
LocationResource::LocationResource() {
    binder_ = nullptr;
}

LocationResource::LocationResource(std::string& baseUri, std::set<cModule*>& eNodeBs, LteBinder *binder) {
    binder_ = binder;
    baseUri_ = baseUri;
	std::set<cModule*>::iterator it = eNodeBs.begin();
	for(; it != eNodeBs.end() ; ++it){
	    LteCellInfo * cellInfo = check_and_cast<LteCellInfo *>((*it)->getSubmodule("cellInfo"));
		eNodeBs_.insert(std::pair<MacCellId, LteCellInfo *>(cellInfo->getMacCellId(), cellInfo));
	}
}

void LocationResource::addEnodeB(std::set<cModule*>& eNodeBs) {
    std::set<cModule*>::iterator it = eNodeBs.begin();
        for(; it != eNodeBs.end() ; ++it){
            LteCellInfo * cellInfo = check_and_cast<LteCellInfo *>((*it)->getSubmodule("cellInfo"));
            eNodeBs_.insert(std::pair<MacCellId, LteCellInfo *>(cellInfo->getMacCellId(), cellInfo));
            EV << "LocationResource::addEnodeB - added eNodeB: " << cellInfo->getMacCellId() << endl;
        }
}

void LocationResource::addEnodeB(cModule* eNodeB) {

    LteCellInfo * cellInfo = check_and_cast<LteCellInfo *>(eNodeB->getSubmodule("cellInfo"));
    eNodeBs_.insert(std::pair<MacCellId, LteCellInfo *>(cellInfo->getMacCellId(), cellInfo));
    EV << "LocationResource::addEnodeB with cellId: "<< cellInfo->getMacCellId() << endl;
    EV << "LocationResource::addEnodeB - added eNodeB: " << cellInfo->getMacCellId() << endl;

}

void LocationResource::addBinder(LteBinder *binder)
{
    binder_= binder;
}

void LocationResource::setBaseUri(const std::string& baseUri)
{
    baseUri_ = baseUri;

}

LocationResource::~LocationResource(){}


UserInfo LocationResource::getUserInfoByNodeId(MacNodeId nodeId, MacCellId cellId) const
{
    // throw exeption if macNodeId does no exist?
    inet::Ipv4Address ipAddress = binder_->getIPv4Address(nodeId);
    std::string refUrl = baseUri_ + "?address=acr:" + ipAddress.str();
    inet::Coord  speed = LocationUtils::getSpeed(nodeId);
    inet::Coord  position = LocationUtils::getCoordinates(nodeId);
    UserInfo ueInfo = UserInfo(position, speed , ipAddress, cellId, refUrl);
    return ueInfo;
}

User LocationResource::getUserByNodeId(MacNodeId nodeId, MacCellId cellId) const
{
    // throw exeption if macNodeId does no exist?
    inet::Ipv4Address ipAddress = binder_->getIPv4Address(nodeId);
    std::string refUrl = baseUri_ + "/users?address=acr:" + ipAddress.str();
    inet::Coord  speed = LocationUtils::getSpeed(nodeId);
    inet::Coord  position = LocationUtils::getCoordinates(nodeId);
    User ueInfo = User(ipAddress, cellId, refUrl);
    return ueInfo;
}




nlohmann::ordered_json LocationResource::getUserListPerCell(std::map<MacCellId, LteCellInfo *>::const_iterator it) const
{
    nlohmann::ordered_json ueArray;
    EV << "LocationResource::toJson() - gnb" << it->first << endl;

    /*
     * this structure take trace of the UE already counted.
     * Each UE can have two MacNodeId, so it would be counted twice.
     * The uniqueness of the ip address allow to avoid count twice
     */
    std::set<inet::Ipv4Address> addressess;
    const std::map<MacNodeId, inet::Coord>* uePositionList;

    uePositionList = it->second->getUePositionList();

    std::map<MacNodeId, inet::Coord>::const_iterator pit = uePositionList->begin();
    std::map<MacNodeId, inet::Coord>::const_iterator end = uePositionList->end();
    for(; pit != end ; ++pit)
    {
        inet::Ipv4Address ipAddress = binder_->getIPv4Address(pit->first);
        if(addressess.find(ipAddress) != addressess.end())
            continue;
        addressess.insert(ipAddress);
        EV << "LocationResource::toJson() - user: " << pit->first << endl;
        User ueInfo = getUserByNodeId(pit->first, it->first);
        if(ueInfo.getIpv4Address() != inet::Ipv4Address::UNSPECIFIED_ADDRESS)
            ueArray.push_back(ueInfo.toJson());
    }

    return ueArray;
}


nlohmann::ordered_json LocationResource::toJson() const {
	nlohmann::ordered_json val ;
	nlohmann::ordered_json userList;
	nlohmann::ordered_json ueArray;
	std::map<MacCellId, LteCellInfo *>::const_iterator it = eNodeBs_.begin();

	for(; it != eNodeBs_.end() ; ++it){
	    ueArray.push_back(getUserListPerCell(it));
	}

	if(ueArray.size() > 1){
		val["user"] = ueArray;
    }
	else if(ueArray.size() == 1){
		val["user"] = ueArray[0];
	}

	userList["userList"] = val;

return userList;
}

//
nlohmann::ordered_json LocationResource::toJsonUe(std::vector<inet::Ipv4Address>& uesID) const {
	nlohmann::ordered_json val ;
	nlohmann::ordered_json ueArray;

	std::vector<inet::Ipv4Address>::const_iterator uit = uesID.begin();
	std::map<MacCellId, LteCellInfo*>::const_iterator eit;
	std::map<MacNodeId, inet::Coord>::const_iterator pit;
	const std::map<MacNodeId, inet::Coord>* uePositionList;
	bool found = false;
	for(; uit != uesID.end() ; ++uit){
	    MacNodeId nodeId = binder_->getMacNodeId(*uit);
	    found = false;
	    eit = eNodeBs_.begin();
	    for(; eit != eNodeBs_.end() ; ++eit){
	        uePositionList = eit->second->getUePositionList();
	        pit = uePositionList->find(nodeId);
	        if(pit != uePositionList->end())
	        {
	            UserInfo ueInfo = getUserInfoByNodeId(pit->first, eit->first);
	            if(ueInfo.getIpv4Address() != inet::Ipv4Address::UNSPECIFIED_ADDRESS)
	                ueArray.push_back(ueInfo.toJson());
                found = true;
                break; // next ue id
	        }
        }
        if(!found)
        {
           std::string notFound = "Address: " + (*uit).str() + " Not found.";
           ueArray.push_back(notFound);
        }
	}
	if(ueArray.size() > 1){
        val["userInfo"] = ueArray;
	}
    else if(ueArray.size() == 1){
        val["userInfo"] = ueArray[0];
    }
	return val;
}


//
nlohmann::ordered_json LocationResource::toJsonCell(std::vector<MacCellId>& cellsID) const
{
    nlohmann::ordered_json val ;
    nlohmann::ordered_json LocationResource;
    nlohmann::ordered_json ueArray;

    std::vector<MacCellId>::const_iterator cid =  cellsID.begin();
    std::map<MacCellId, LteCellInfo *>::const_iterator it;
    const std::map<MacNodeId, inet::Coord>* uePositionList;

    for(; cid != cellsID.end() ; ++cid){
        it = eNodeBs_.find(*cid);
        if(it != eNodeBs_.end()){
            ueArray.push_back(getUserListPerCell(it));
        }
        else
        {
            std::string notFound = "AccessPointId: " + std::to_string(*cid) + " Not found.";
            ueArray.push_back(notFound);
        }

    }

    if(ueArray.size() > 1){
        val["user"] = ueArray;
    }
    else if(ueArray.size() == 1){
        val["user"] = ueArray[0];
    }

    LocationResource["userList"] = val;
    return LocationResource;
}
////
nlohmann::ordered_json LocationResource::toJson(std::vector<MacCellId>& cellsID, std::vector<inet::Ipv4Address>& uesID) const
{
    nlohmann::ordered_json val ;
//    nlohmann::ordered_json LocationResource;
//	val["cellInfo"] = toJsonCell(cellsID)["LocationResource"]["cellInfo"];
//	val["CellUEInfo"] = toJsonUe(uesID)["LocationResource"]["CellUEInfo"];
//	LocationResource["LocationResource"] = val;
//	return LocationResource;
    return val;
}






