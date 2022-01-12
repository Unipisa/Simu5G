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

#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/L2MeasSubscription.h"
#include <iostream>

using namespace omnetpp;

L2MeasSubscription::L2MeasSubscription() :SubscriptionBase() {};
L2MeasSubscription::L2MeasSubscription(unsigned int subId, inet::TcpSocket *socket, const std::string& baseResLocation,  std::set<cModule*>& eNodeBs):
        SubscriptionBase(subId,socket,baseResLocation, eNodeBs){};
L2MeasSubscription:: ~L2MeasSubscription(){};

bool L2MeasSubscription::fromJson(const nlohmann::ordered_json& body)
{
    if(body.contains("L2MeasurementSubscription")) // mandatory attribute
    {

        subscriptionType_ = "L2MeasurementSubscription";
    }
    else
    {
        Http::send400Response(socket_); // callbackReference is mandatory and takes exactly 1 att
        return false;
    }

    nlohmann::ordered_json jsonBody = body["L2MeasurementSubscription"];

    // add basis information
    bool result = SubscriptionBase::fromJson(jsonBody);
    // add information relative to this type of subscription
    if(result)
    {
        callbackReference_ += "notifications/"+ std::to_string(subscriptionId_);

        if(!jsonBody.contains("filterCriteria") || jsonBody["filterCriteria"].is_array())
        {
            std::cout << "1"  << std::endl;
           Http::send400Response(socket_); // filterCriteria is mandatory and takes exactly 1 att
           return false;
        }


        nlohmann::json filterCriteria = jsonBody["filterCriteria"];

        //check for appInstanceId filter
        if(filterCriteria.contains("appInstanceId")  )
        {
            if(filterCriteria["appInstanceId"].is_array())
            {
                std::cout << "2"  << std::endl;

                Http::send400Response(socket_); // appInstanceId, if present, takes exactly 1 att
                return false;
            }
            filterCriteria_.appIstanceId = filterCriteria["appInstanceId"];
        }

        //check ues filter
        if(filterCriteria.contains("associateId"))
        {
            if(filterCriteria["associateId"].is_array())
            {
                std::cout << "3"  << std::endl;

                Http::send400Response(socket_); // only one ip
                return false;
//                nlohmann::json ueVector = filterCriteria["associateId"];
//                for(int i = 0; i < ueVector.size(); ++i)
//                {
//                    if(ueVector.at(i)["associateId"]["type"] == "UE_IPv4_ADDRESS")
//                    {
//                        std::string address = ueVector.at(i)["associateId"]["value"];
//                        ues.push_back(binder_->getMacNodeId(IPv4Address(address.c_str())));
//                    }
//                    else
//                    {
//                        Http::send400Response(socket_); // must be ipv4
//                        return false;
//                     }
//                 }
            }
            else
            {
                if(filterCriteria["associateId"]["type"] == "UE_IPv4_ADDRESS")
                {
                    filterCriteria_.associteId_.setType(filterCriteria["associateId"]["type"]);
                    filterCriteria_.associteId_.setValue(filterCriteria["associateId"]["value"]);

                }
            }
        }
        else
        {
            std::cout << "4"  << std::endl;

            Http::send400Response(socket_); // a user must be indicated
            return false;
        }

        //check cellIds filter
        if(filterCriteria.contains("ecgi"))
        {
            if(filterCriteria["ecgi"].is_array())
            {
//                nlohmann::json cellVector = filterCriteria["cellId"];
//                for(int i = 0; i < cellVector.size(); ++i)
//                {
//                    std::string cellId = cellVector.at(i)["cellId"];
//                    cellids.push_back((MacNodeId)std::stoi(cellId));
//                 }
            }
            else
            {
                if(filterCriteria["ecgi"].contains("cellId") && filterCriteria["ecgi"].contains("plmn"))
                {
                    std::string cellId = filterCriteria["ecgi"]["cellId"];
                    filterCriteria_.ecgi.setCellId((MacNodeId)std::stoi(cellId));
                    mec::Plmn plmn;
                    plmn.mcc = filterCriteria["ecgi"]["plmn"]["mcc"];
                    plmn.mnc = filterCriteria["ecgi"]["plmn"]["mnc"];
                    filterCriteria_.ecgi.setPlmn(plmn);
                }
                else
                {
                    std::cout << "5"  << std::endl;

                    Http::send400Response(socket_); // a user must be indicated
                    return false;
                }

            }
        }

        //check trigger filter
        if(filterCriteria.contains("trigger"))
        {
            //check if it is event trigger and notify, based on the state of the ues e cells
        }

        nlohmann::ordered_json response = body;
        response[subscriptionType_]["callbackReference"] = callbackReference_;
        response[subscriptionType_]["_links"]["self"] = links_;

        std::pair<std::string, std::string> p("Location: ", links_);
        Http::send201Response(socket_, response.dump(2).c_str(), p );
        return true;
    }

    return false;
}
void L2MeasSubscription::sendSubscriptionResponse(){
        nlohmann::ordered_json val;
        val[subscriptionType_]["callbackReference"] = callbackReference_;
        val[subscriptionType_]["_links"]["self"] = links_;
        val[subscriptionType_]["filterCriteria"] = filterCriteria_.associteId_.toJson();
        val[subscriptionType_]["filterCriteria"] = filterCriteria_.ecgi.toJson();

}
void L2MeasSubscription::sendNotification(EventNotification *event){}



