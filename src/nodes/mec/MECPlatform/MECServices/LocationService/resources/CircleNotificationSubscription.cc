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

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/CircleNotificationSubscription.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/CurrentLocation.h"
#include "common/LteCommon.h"
#include "common/cellInfo/CellInfo.h"
#include "common/binder/Binder.h"
#include "inet/mobility/base/MovingMobilityBase.h"
#include "nodes/mec/MECPlatform/EventNotification/CircleNotificationEvent.h"
using namespace omnetpp;

CircleNotificationSubscription::CircleNotificationSubscription()
{
 binder = getBinder();
 firstNotificationSent = false;
 lastNotification = 0;
}

CircleNotificationSubscription::CircleNotificationSubscription(unsigned int subId, inet::TcpSocket *socket , const std::string& baseResLocation,  std::set<cModule*>& eNodeBs):
SubscriptionBase(subId,socket,baseResLocation, eNodeBs){
    binder = getBinder();
    baseResLocation_+= "area/circle";
    firstNotificationSent = false;
};

CircleNotificationSubscription::CircleNotificationSubscription(unsigned int subId, inet::TcpSocket *socket , const std::string& baseResLocation,  std::set<cModule*>& eNodeBs, bool firstNotSent,  omnetpp::simtime_t lastNot):
SubscriptionBase(subId,socket,baseResLocation, eNodeBs){
    binder = getBinder();
    baseResLocation_+= "area/circle";
    firstNotificationSent = firstNotSent;
    lastNotification = lastNot;
};

CircleNotificationSubscription::~CircleNotificationSubscription(){
}


void CircleNotificationSubscription::sendSubscriptionResponse(){}


void CircleNotificationSubscription::sendNotification(EventNotification *event)
{
    EV << "CircleNotificationSubscription::sendNotification" << endl;

    EV << firstNotificationSent << " last " << lastNotification << " now " << simTime() << " frequency" << frequency << endl;
    if(firstNotificationSent && (simTime() - lastNotification) <= frequency)
    {
        EV <<"CircleNotificationSubscription::sendNotification - notification event occured near the last one. Frequency for notifications is: " << frequency << endl;
        return;
    }

    CircleNotificationEvent *circleEvent = check_and_cast<CircleNotificationEvent*>(event);

    nlohmann::ordered_json val;
    nlohmann::ordered_json terminalLocationArray;


    val["enteringLeavingCriteria"] = actionCriteria == LocationUtils::Entering? "Entering" : "Leaving";
    val["isFinalNotification"] = "false";
    val["link"]["href"] = resourceURL;
    val["link"]["rel"] = subscriptionType_;

    // std::vector<TerminalLocation>::const_iterator it = terminalLocations.begin();
    std::vector<TerminalLocation> terminalLoc = circleEvent->getTerminalLocations();

    for(auto it : terminalLoc)
    {
        terminalLocationArray.push_back(it.toJson());
    }
    if(terminalLocationArray.size() > 1)
        val["terminalLocationList"] = terminalLocationArray;
    else
        val["terminalLocationList"] = terminalLocationArray[0];


    nlohmann::ordered_json notification;
    notification["subscriptionNotification"] = val;

//    if(socket_->getState())
//        throw cRuntimeError("%d", socket_->getState());
    Http::sendPostRequest(socket_, notification.dump(2).c_str(), clientHost_.c_str(), clientUri_.c_str());

    // update last notification sent
    lastNotification = simTime();
    if(firstNotificationSent == false)
        firstNotificationSent = true;
}

bool CircleNotificationSubscription::fromJson(const nlohmann::ordered_json& body)
{

    // ues; // optional: NO
    //
    ////callbackReference
    // callbackData;// optional: YES
    // notifyURL; // optional: NO
    //
    // checkImmediate; // optional: NO
    // clientCorrelator; // optional: YES
    //
    // frequency; // optional: NO
    // center; // optional: NO
    // radius; // optional: NO
    // trackingAccuracy; // optional: NO
    // EnteringLeavingCriteria; // optional: NO

    if(body.contains("circleNotificationSubscription")) // mandatory attribute
    {

        subscriptionType_ = "circleNotificationSubscription";
    }
    else
    {
        EV << "1" << endl;
        Http::send400Response(socket_, "circleNotificationSubscription JSON name is mandatory");
        return false;
    }
    nlohmann::ordered_json jsonBody = body["circleNotificationSubscription"];

    if(jsonBody.contains("callbackReference"))
    {
        nlohmann::ordered_json callbackReference = jsonBody["callbackReference"];
        if(callbackReference.contains("callbackData"))
            callbackData = callbackReference["callbackData"];
        if(callbackReference.contains("notifyURL"))
        {
            notifyURL = callbackReference["notifyURL"];
            // parse it to retreive the resource uri and
            // the host
            std::size_t found = notifyURL.find("/");
          if (found!=std::string::npos)
          {
              clientHost_ = notifyURL.substr(0, found);
              clientUri_ = notifyURL.substr(found);
          }

        }
        else
        {
            EV << "2" << endl;

            Http::send400Response(socket_); //notifyUrl is mandatory
            return false;
        }

    }
    else
    {
        // callbackReference is mandatory and takes exactly 1 value
        Http::send400Response(socket_, "callbackReference JSON name is mandatory");
        return false;
    }

    if(jsonBody.contains("checkImmediate"))
    {
        std::string check = jsonBody["checkImmediate"];
        checkImmediate = check.compare("true") == 0 ? true : false;
    }
    else
    {
        checkImmediate = false;
    }

    if(jsonBody.contains("clientCorrelator"))
    {
       clientCorrelator = jsonBody["clientCorrelator"];
    }

    if(jsonBody.contains("frequency"))
    {
       frequency = jsonBody["frequency"];
    }
    else
   {
        EV << "4" << endl;
        Http::send400Response(socket_, "frequency JSON name is mandatory");
       return false;
   }

    if(jsonBody.contains("radius"))
    {
        radius = jsonBody["radius"];
    }
    else
    {
        Http::send400Response(socket_, "radius JSON name is mandatory");
        return false;
    }

    if(jsonBody.contains("center") || (jsonBody.contains("latitude") && jsonBody.contains("longitude")))
    {
        if(jsonBody.contains("center"))
        {
            center.x = jsonBody["center"]["x"];
            center.y = jsonBody["center"]["y"];
            center.z = jsonBody["center"]["z"];
        }

        else
        {
            latitude = jsonBody["latitude"]; //  y in the simulator
            longitude = jsonBody["longitude"]; // x in the simulator
        }

    }
    else
   {
       Http::send400Response(socket_, "Position coordinates JSON names are mandatory");
       return false;
   }

    if(jsonBody.contains("trackingAccuracy"))
    {
        trackingAccuracy = jsonBody["trackingAccuracy"];
    }
    else
   {
        Http::send400Response(socket_, "trackingAccuracy JSON name is mandatory");//trackingAccuracy is mandatory
        return false;
    }

    if(jsonBody.contains("enteringLeavingCriteria"))
    {
        std::string criteria = jsonBody["enteringLeavingCriteria"];
        if(criteria.compare("Entering") == 0)
        {
            actionCriteria = LocationUtils::Entering;
        }
        else if(criteria.compare("Leaving") == 0)
        {
            actionCriteria = LocationUtils::Leaving;
        }

        //get the current state of the ue

    }
    else
   {
       Http::send400Response(socket_, "enteringLeavingCriteria JSON name is mandatory");//trackingAccuracy is mandatory
       return false;
   }

    if(jsonBody.contains("address"))
    {
        if(jsonBody["address"].is_array())
        {
            nlohmann::json addressVector = jsonBody["address"];
            for(int i = 0; i < addressVector.size(); ++i)
            {
                std::string add =  addressVector.at(i);
                MacNodeId id = binder->getMacNodeId(inet::Ipv4Address(add.c_str()));
                /*
                 * check if the address is already present in the network.
                 * If not, do anything. Maybe such address will be available later.
                 * OR make a 400 response telling one address is not available?
                 */

                if(id == 0 || !findUe(id))
                {
                    EV << "IP NON ESISTE" << endl;
                    Http::send400Response(socket_); //address is mandatory
                    return false;
                   //TODO cosa fare in caso in cui un address non esiste?
                }
                else
                {
                // set the initial state
                    inet::Coord coord = LocationUtils::getCoordinates(id);
                    inet::Coord center = inet::Coord(latitude,longitude,0.);
                    EV << "center: [" << latitude << ";"<<longitude << "]"<<endl;
                    EV << "coord: [" << coord.x << ";"<<coord.y << "]"<<endl;
                    EV << "distance: " << coord.distance(center) <<endl;

                    users[id] = (coord.distance(center) <= radius) ? true : false; // true = inside
                }
            }
        }
        else
        {
            std::string add =  jsonBody["address"];
            MacNodeId id = binder->getMacNodeId(inet::Ipv4Address(add.c_str()));
            if(id == 0 || !findUe(id))
            {
                EV << "IP NON ESISTE" << endl;
            }
            else
            {
                // set the initial state
                inet::Coord coord = LocationUtils::getCoordinates(id);
                inet::Coord center = inet::Coord(latitude,longitude,0.);
                EV << "center: [" << latitude << ";"<<longitude << "]"<<endl;
                EV << "coord: [" << coord.x << ";"<<coord.y << "]"<<endl;
                EV << "distance: " << coord.distance(center) <<endl;
                users[id] = (coord.distance(center) <= radius) ? true : false;
            }

        }
    }
    else
    {
        Http::send400Response(socket_, "address JSON name is mandatory");//address is mandatory
        return false;
    }


    resourceURL = baseResLocation_+ "/" + std::to_string(subscriptionId_);
    return true;
}

EventNotification* CircleNotificationSubscription::handleSubscription()
{

    EV << "CircleNotificationSubscription::handleSubscription()" << endl;
    terminalLocations.clear();
    std::map<MacNodeId, bool>::iterator it = users.begin();
    for(; it != users.end(); ++it)
    {
        bool found = false;
        //check if the use is under one of the Enodeb connected to the Mehost

        if(!findUe(it->first))
            continue; // TODO manage what to do
        inet::Coord coord = LocationUtils::getCoordinates(it->first);
        inet::Coord center = inet::Coord(latitude,longitude,0.);
//        EV << "center: [" << latitude << ";"<<longitude << "]"<<endl;
//        EV << "coord: [" << coord.x << ";"<<coord.y << "]"<<endl;
//        EV << "distance: " << coord.distance(center) <<endl;

        if(actionCriteria == LocationUtils::Entering)
        {
            if(coord.distance(center) <= radius && it->second == false){
                it->second = true;
                EV << "dentro" << endl;
                found = true;
            }
            else if (coord.distance(center) >= radius)
            {
                it->second = false;
            }
        }
        else
        {
            if(coord.distance(center) >= radius && it->second == true){
                it->second = false;
                EV << "fuori" << endl;
                found = true;
            }
            else if (coord.distance(center) <= radius)
            {
                it->second = true;
            }

        }

        if(found)
        {
            std::string status = "Retrieved";
            CurrentLocation location(100, coord);
            TerminalLocation user(binder->getIPv4Address(it->first).str(), status, location);
            terminalLocations.push_back(user);
        }
    }
    if(!terminalLocations.empty())
    {
        CircleNotificationEvent *notificationEvent = new CircleNotificationEvent(subscriptionType_,subscriptionId_, terminalLocations);
        return notificationEvent;
    }
    else
        return nullptr;

}

bool CircleNotificationSubscription::findUe(MacNodeId nodeId)
{
    std::map<MacCellId, CellInfo*>::const_iterator  eit = eNodeBs_.begin();
    std::map<MacNodeId, inet::Coord>::const_iterator pit;
    // const std::map<MacNodeId, inet::Coord>* uePositionList;
    for(; eit != eNodeBs_.end() ; ++eit){
        inet::Coord uePos = eit->second->getUePosition(nodeId);
       if(uePos != inet::Coord::ZERO)
       {
           return true;
       }
    }
    return false;
}
