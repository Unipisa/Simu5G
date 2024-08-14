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

#ifndef _L2MEAS_H_
#define _L2MEAS_H_

#include "common/utils/utils.h"
#include "common/LteCommon.h"
#include <vector>
#include <map>
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"

namespace simu5g {

using namespace omnetpp;

class BaseStationStatsCollector;
class Binder;

/*
 * This class is responsible for retrieving the L2 measures and creating the JSON body
 * response. It maintains the list of all the eNB/gNBs associated with the MEC host
 * where the RNIS is running and keeps the pointers to the s.*
 */

class L2Meas : public AttributeBase
{
  public:
    L2Meas();
    L2Meas(std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs);

    void setBinder(Binder *binder) { binder_ = binder; }

    nlohmann::ordered_json toJson() const override;

    void addEnodeB(std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs);
    void addEnodeB(cModule *eNodeB);

    nlohmann::ordered_json toJsonCell(std::vector<MacCellId>& cellsID) const;
    nlohmann::ordered_json toJsonUe(std::vector<inet::Ipv4Address>& uesID) const;
    nlohmann::ordered_json toJson(std::vector<MacNodeId>& cellsID, std::vector<inet::Ipv4Address>& uesID) const;

  protected:

    TimeStamp timestamp_;
    std::map<MacCellId, BaseStationStatsCollector *> eNodeBs_;
    opp_component_ptr<Binder> binder_;
};

} //namespace

#endif // _L2MEAS_H_

