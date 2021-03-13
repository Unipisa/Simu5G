/*
 * UserInfo.h
 *
 *  Created on: Dec 5, 2020
 *      Author: linofex
 */

#ifndef CORENETWORK_NODES_MEC_MEPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_USERINFO_H_
#define CORENETWORK_NODES_MEC_MEPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_USERINFO_H_

#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/LocationInfo.h"
#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"
#include "nodes/mec/MEPlatform/MeServices/Resources/TimeStamp.h"

class UserInfo : public AttributeBase
{
    public:
        /**
         * the constructor takes a vector of the eNodeBs connected to the MeHost
         * and creates a CellInfo object
        */
        UserInfo();
        UserInfo(const LocationInfo& location, const std::string& address, const MacCellId accessPointId_, const std::string& resourceUrl_);
        UserInfo(const inet::Coord& location, const inet::Coord& speed,  const std::string& address, const MacCellId accessPointId_,  const std::string& resourceUrl_);

        virtual ~UserInfo(){};

        nlohmann::ordered_json toJson() const override;

    protected:
        TimeStamp timestamp_;
        std::string address_;
        MacCellId accessPointId_;
        std::string resourceUrl_;
        LocationInfo locationInfo_;
};



#endif /* CORENETWORK_NODES_MEC_MEPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_USERINFO_H_ */
