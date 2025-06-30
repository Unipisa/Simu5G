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

#ifndef _LOCATION_H_
#define _LOCATION_H_

#include <map>
#include <set>

#include <inet/networklayer/contract/ipv4/Ipv4Address.h>

#include "common/LteCommon.h"
#include "common/binder/Binder.h"
#include "common/utils/utils.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/UserInfo.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"

namespace simu5g {

using namespace omnetpp;

class CellInfo;

class LocationResource : public AttributeBase
{
  public:
    /**
     * The constructor takes a vector of the eNodeBs connected to the MeHost
     * and creates a CellInfo object
     */
    LocationResource();
    LocationResource(const std::string& baseUri, std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs, Binder *binder);

    nlohmann::ordered_json toJson() const override;

    void addEnodeB(std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs);
    void addEnodeB(cModule *eNodeB);
    void addBinder(Binder *binder);
    void setBaseUri(const std::string& baseUri);
    nlohmann::ordered_json toJsonCell(std::vector<MacCellId>& cellsID) const;
    nlohmann::ordered_json toJsonUe(std::vector<inet::Ipv4Address>& uesID) const;
    nlohmann::ordered_json toJson(std::vector<MacNodeId>& cellsID, std::vector<inet::Ipv4Address>& uesID) const;

  protected:
    // better to map <cellID, CellInfo>
    opp_component_ptr<Binder> binder_;
    TimeStamp timestamp_;
    std::map<MacCellId, CellInfo *> eNodeBs_;
    std::string baseUri_;

    UserInfo getUserInfoByNodeId(MacNodeId nodeId, MacCellId cellId) const;
    User getUserByNodeId(MacNodeId nodeId, MacCellId cellId) const;

    nlohmann::ordered_json getUserListPerCell(MacCellId macCellId, CellInfo *cellInfo) const;

};

} //namespace

#endif // _LOCATION_H_

