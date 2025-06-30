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

#ifndef CORENETWORK_NODES_MEC_MECPLATFORM_MEC_SERVICES_LOCATION_SERVICE_RESOURCES_USER_H_
#define CORENETWORK_NODES_MEC_MECPLATFORM_MEC_SERVICES_LOCATION_SERVICE_RESOURCES_USER_H_

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationInfo.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"

namespace simu5g {

class User : public AttributeBase
{
  public:
    /**
     * The constructor takes a vector of the eNodeBs connected to the MeHost
     * and creates a CellInfo object
     */
    User();
    User(const inet::Ipv4Address& address, const MacCellId accessPointId, const std::string& resourceUrl, int zoneId = 0);


    inet::Ipv4Address getIpv4Address() const { return address_; }

    nlohmann::ordered_json toJson() const override;
    void setAccessPointId(MacCellId cellId) { accessPointId_ = cellId; }

  protected:
    TimeStamp timestamp_;
    inet::Ipv4Address address_;
    int zoneId_;
    MacCellId accessPointId_;
    std::string resourceUrl_;
};

} //namespace

#endif /* CORENETWORK_NODES_MEC_MECPLATFORM_MEC_SERVICES_LOCATION_SERVICE_RESOURCES_USER_H_ */

