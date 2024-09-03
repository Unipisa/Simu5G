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

#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"
#include "common/cellInfo/CellInfo.h"

namespace simu5g {

using namespace omnetpp;

SubscriptionBase::SubscriptionBase() {}

SubscriptionBase::SubscriptionBase(unsigned int subId, inet::TcpSocket *socket, const std::string& baseResLocation, std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs) {
    for (cModule *eNodeB : eNodeBs) {
        CellInfo *cellInfo = check_and_cast<CellInfo *>(eNodeB->getSubmodule("cellInfo"));
        eNodeBs_.insert({cellInfo->getMacCellId(), cellInfo});
    }
    subscriptionId_ = subId;
    socket_ = socket;
    baseResLocation_ = baseResLocation;
//	notificationTrigger = nullptr;
}

void SubscriptionBase::addEnodeB(std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs) {
    for (auto* module : eNodeBs) {
        CellInfo *cellInfo = check_and_cast<CellInfo *>(module->getSubmodule("cellInfo"));
        eNodeBs_.insert({cellInfo->getMacCellId(), cellInfo});
        EV << "LocationResource::addEnodeB - added eNodeB: " << cellInfo->getMacCellId() << endl;
    }
}

void SubscriptionBase::addEnodeB(cModule *eNodeB) {
    CellInfo *cellInfo = check_and_cast<CellInfo *>(eNodeB->getSubmodule("cellInfo"));
    eNodeBs_.insert({cellInfo->getMacCellId(), cellInfo});
    EV << "LocationResource::addEnodeB with cellId: " << cellInfo->getMacCellId() << endl;
    EV << "LocationResource::addEnodeB - added eNodeB: " << cellInfo->getMacCellId() << endl;
}

SubscriptionBase::~SubscriptionBase() {}

bool SubscriptionBase::fromJson(const nlohmann::ordered_json& jsonBody)
{
    if (!jsonBody.contains("callbackReference") || jsonBody["callbackReference"].is_array()) {
        Http::send400Response(socket_); // callbackReference is mandatory and takes exactly 1 argument
        return false;
    }

    if (std::string(jsonBody["callbackReference"]).find('/') == std::string::npos) { //bad uri
        Http::send400Response(socket_); // must be ipv4
        return false;
    }

    callbackReference_ = jsonBody["callbackReference"];

    //check expiration time
    // TODO add end timer
    if (jsonBody.contains("expiryDeadline") && !jsonBody["expiryDeadline"].is_array()) {
        expiryTime_.setSeconds(jsonBody["expiryDeadline"]["seconds"]);
        expiryTime_.setNanoSeconds(jsonBody["expiryDeadline"]["nanoSeconds"]);
        expiryTime_.setValid(true);
    }
    else {
        expiryTime_.setValid(false);
    }

    links_ = baseResLocation_ + std::to_string(subscriptionId_);

    return true;
}

void SubscriptionBase::set_links(const std::string& link)
{
    links_ = link + "sub" + std::to_string(subscriptionId_);
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

} //namespace

