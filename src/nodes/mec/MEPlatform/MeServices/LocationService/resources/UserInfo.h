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
#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/User.h"


class UserInfo : public AttributeBase
{
    public:
        /**
         * the constructor takes a vector of the eNodeBs connected to the MeHost
         * and creates a CellInfo object
        */
        UserInfo();
        UserInfo(const LocationInfo& location, const inet::Ipv4Address& address, const MacCellId accessPointId_, const std::string& resourceUrl_, int zoneId = 0);
        UserInfo(const inet::Coord& location, const inet::Coord& speed,  const inet::Ipv4Address& address, const MacCellId accessPointId_,  const std::string& resourceUrl, int zoneId = 0);

        virtual ~UserInfo(){};

        inet::Ipv4Address getIpv4Address() const {return address_;}

        nlohmann::ordered_json toJson() const override;
        void setAccessPointId(MacCellId cellId) {accessPointId_ = cellId;}

    protected:
        TimeStamp timestamp_;
        inet::Ipv4Address address_;
        MacCellId accessPointId_;
        int zoneId_;
        std::string resourceUrl_;
        LocationInfo locationInfo_;
};



#endif /* CORENETWORK_NODES_MEC_MEPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_USERINFO_H_ */
