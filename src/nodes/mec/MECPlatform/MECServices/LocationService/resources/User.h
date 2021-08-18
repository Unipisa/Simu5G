/*
 * User.h
 *
 *  Created on: Dec 5, 2020
 *      Author: linofex
 */

#ifndef CORENETWORK_NODES_MEC_MECPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_USER_H_
#define CORENETWORK_NODES_MEC_MECPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_USER_H_

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationInfo.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"

class User : public AttributeBase
{
    public:
        /**
         * the constructor takes a vector of the eNodeBs connected to the MeHost
         * and creates a CellInfo object
        */
        User();
        User(const inet::Ipv4Address& address, const MacCellId accessPointId, const std::string& resourceUrl, int zoneId = 0);

        virtual ~User(){};

        inet::Ipv4Address getIpv4Address() const {return address_;}

        nlohmann::ordered_json toJson() const override;
        void setAccessPointId(MacCellId cellId) {accessPointId_ = cellId;}

    protected:
        TimeStamp timestamp_;
        inet::Ipv4Address address_;
        int zoneId_;
        MacCellId accessPointId_;
        std::string resourceUrl_;

};



#endif /* CORENETWORK_NODES_MEC_MECPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_USER_H_ */
