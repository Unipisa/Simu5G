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

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationResource.h"

#include <inet/mobility/base/MovingMobilityBase.h>

#include "common/binder/Binder.h"
#include "common/cellInfo/CellInfo.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationApiDefs.h"

namespace simu5g {

LocationResource::LocationResource() : binder_(nullptr) {
}

LocationResource::LocationResource(const std::string& baseUri, std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs, Binder *binder) : binder_(binder), baseUri_(baseUri) {
    for (auto* eNodeB : eNodeBs) {
        CellInfo *cellInfo = check_and_cast<CellInfo *>((eNodeB)->getSubmodule("cellInfo"));
        eNodeBs_.insert({cellInfo->getMacCellId(), cellInfo});
    }
}

void LocationResource::addEnodeB(std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs) {
    for (auto& eNodeB : eNodeBs) {
        CellInfo *cellInfo = check_and_cast<CellInfo *>(eNodeB->getSubmodule("cellInfo"));
        eNodeBs_.insert({cellInfo->getMacCellId(), cellInfo});
        EV << "LocationResource::addEnodeB - added eNodeB: " << cellInfo->getMacCellId() << endl;
    }
}

void LocationResource::addEnodeB(cModule *eNodeB) {
    CellInfo *cellInfo = check_and_cast<CellInfo *>(eNodeB->getSubmodule("cellInfo"));
    eNodeBs_.insert({cellInfo->getMacCellId(), cellInfo});
    EV << "LocationResource::addEnodeB with cellId: " << cellInfo->getMacCellId() << endl;
    EV << "LocationResource::addEnodeB - added eNodeB: " << cellInfo->getMacCellId() << endl;
}

void LocationResource::addBinder(Binder *binder)
{
    binder_ = binder;
}

void LocationResource::setBaseUri(const std::string& baseUri)
{
    baseUri_ = baseUri;
}


UserInfo LocationResource::getUserInfoByNodeId(MacNodeId nodeId, MacCellId cellId) const
{
    // throw exception if macNodeId does not exist?
    inet::Ipv4Address ipAddress = binder_->getIPv4Address(nodeId);
    std::string refUrl = baseUri_ + "?address=acr:" + ipAddress.str();
    inet::Coord speed = LocationUtils::getSpeed(binder_, nodeId);
    inet::Coord position = LocationUtils::getCoordinates(binder_, nodeId);
    UserInfo ueInfo = UserInfo(position, speed, ipAddress, cellId, refUrl);
    return ueInfo;
}

User LocationResource::getUserByNodeId(MacNodeId nodeId, MacCellId cellId) const
{
    // throw exception if macNodeId does not exist?
    inet::Ipv4Address ipAddress = binder_->getIPv4Address(nodeId);
    std::string refUrl = baseUri_ + "/users?address=acr:" + ipAddress.str();
    User ueInfo = User(ipAddress, cellId, refUrl);
    return ueInfo;
}

nlohmann::ordered_json LocationResource::getUserListPerCell(MacCellId macCellId, CellInfo *cellInfo) const
{
    nlohmann::ordered_json ueArray;
    EV << "LocationResource::toJson() - gnb" << macCellId << endl;

    /*
     * this structure takes trace of the UE already counted.
     * Each UE can have two MacNodeId, so it would be counted twice.
     * The uniqueness of the ip address allows us to avoid counting twice.
     */
    std::set<inet::Ipv4Address> addressess;
    const std::map<MacNodeId, inet::Coord> *uePositionList;

    uePositionList = cellInfo->getUePositionList();
    for (const auto& item : *uePositionList) {
        inet::Ipv4Address ipAddress = binder_->getIPv4Address(item.first);
        if (addressess.find(ipAddress) != addressess.end())
            continue;
        addressess.insert(ipAddress);
        EV << "LocationResource::toJson() - user: " << item.first << endl;
        User ueInfo = getUserByNodeId(item.first, macCellId);
        if (ueInfo.getIpv4Address() != inet::Ipv4Address::UNSPECIFIED_ADDRESS)
            ueArray.push_back(ueInfo.toJson());
    }

    return ueArray;
}

nlohmann::ordered_json LocationResource::toJson() const {
    nlohmann::ordered_json val;
    nlohmann::ordered_json userList;
    nlohmann::ordered_json ueArray;

    for (auto [cid, cellInfo] : eNodeBs_) {
        ueArray.push_back(getUserListPerCell(cid, cellInfo));
    }

    if (ueArray.size() > 1) {
        val["user"] = ueArray;
    }
    else if (ueArray.size() == 1) {
        val["user"] = ueArray[0];
    }

    userList["userList"] = val;

    return userList;
}

nlohmann::ordered_json LocationResource::toJsonUe(std::vector<inet::Ipv4Address>& uesID) const {
    nlohmann::ordered_json val;
    nlohmann::ordered_json ueArray;

    for (const auto& ueId : uesID) {
        MacNodeId nodeId = binder_->getMacNodeId(ueId);
        bool found = false;
        for (const auto& [macCellId, eNodeB] : eNodeBs_) {
            const std::map<MacNodeId, inet::Coord>* uePositionList = eNodeB->getUePositionList();
            auto pit = uePositionList->find(nodeId);
            if (pit != uePositionList->end()) {
                UserInfo ueInfo = getUserInfoByNodeId(pit->first, macCellId);
                if (ueInfo.getIpv4Address() != inet::Ipv4Address::UNSPECIFIED_ADDRESS)
                    ueArray.push_back(ueInfo.toJson());
                found = true;
                break; // next ue id
            }
        }
        if (!found) {
            std::string notFound = "Address: " + ueId.str() + " Not found.";
            ueArray.push_back(notFound);
        }
    }
    if (ueArray.size() > 1) {
        val["userInfo"] = ueArray;
    }
    else if (ueArray.size() == 1) {
        val["userInfo"] = ueArray[0];
    }
    return val;
}

//
nlohmann::ordered_json LocationResource::toJsonCell(std::vector<MacCellId>& cellsID) const
{
    nlohmann::ordered_json val;
    nlohmann::ordered_json LocationResource;
    nlohmann::ordered_json ueArray;

    for (auto cid : cellsID) {
        auto it = eNodeBs_.find(cid);
        if (it != eNodeBs_.end()) {
            ueArray.push_back(getUserListPerCell(it->first, it->second));
        }
        else {
            std::string notFound = "AccessPointId " + std::to_string(num(cid)) + " Not found.";
            ueArray.push_back(notFound);
        }
    }

    if (ueArray.size() > 1) {
        val["user"] = ueArray;
    }
    else if (ueArray.size() == 1) {
        val["user"] = ueArray[0];
    }

    LocationResource["userList"] = val;
    return LocationResource;
}

nlohmann::ordered_json LocationResource::toJson(std::vector<MacCellId>& cellsID, std::vector<inet::Ipv4Address>& uesID) const
{
    nlohmann::ordered_json val;
    return val;
}

} //namespace

