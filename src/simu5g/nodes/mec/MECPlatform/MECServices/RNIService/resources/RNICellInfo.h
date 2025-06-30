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

#ifndef _RNICELLINFO_H_
#define _RNICELLINFO_H_

#include <vector>
#include <omnetpp.h>
#include <map>
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/Ecgi.h"
#include "corenetwork/statsCollector/UeStatsCollector.h"
#include "corenetwork/statsCollector/BaseStationStatsCollector.h"

namespace simu5g {

class RNICellInfo : public AttributeBase
{
  protected:
    opp_component_ptr<BaseStationStatsCollector> collector_; // it has the cellCollector and the map <Ipue -> uecollector>
    Ecgi ecgi_;

    nlohmann::ordered_json toJsonCell() const; // should this be private?

  public:
    RNICellInfo();
    RNICellInfo(BaseStationStatsCollector *eNodeB);

    UeStatsCollectorMap *getCollectorMap() const;

    Ecgi getEcgi() const;

    nlohmann::ordered_json toJson() const override;
};

} //namespace

#endif // _RNICELLINFO_H_

