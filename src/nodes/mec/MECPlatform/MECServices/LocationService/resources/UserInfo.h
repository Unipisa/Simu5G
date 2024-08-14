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

#ifndef CORENETWORK_NODES_MEC_MEPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_USERINFO_H_
#define CORENETWORK_NODES_MEC_MEPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_USERINFO_H_

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationInfo.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/User.h"

namespace simu5g {

class UserInfo : public AttributeBase
{
  public:
    /**
     * The constructor takes a vector of the eNodeBs connected to the MeHost
     * and creates a CellInfo object
     */
    UserInfo();
    UserInfo(const LocationInfo& location, const inet::Ipv4Address& address, const MacCellId accessPointId_, const std::string& resourceUrl_, int zoneId = 0);
    UserInfo(const inet::Coord& location, const inet::Coord& speed, const inet::Ipv4Address& address, const MacCellId accessPointId_, const std::string& resourceUrl, int zoneId = 0);


    inet::Ipv4Address getIpv4Address() const { return address_; }

    nlohmann::ordered_json toJson() const override;
    void setAccessPointId(MacCellId cellId) { accessPointId_ = cellId; }

  protected:
    TimeStamp timestamp_;
    inet::Ipv4Address address_;
    MacCellId accessPointId_;
    int zoneId_;
    std::string resourceUrl_;
    LocationInfo locationInfo_;
};

} //namespace

#endif /* CORENETWORK_NODES_MEC_MEPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_USERINFO_H_ */

