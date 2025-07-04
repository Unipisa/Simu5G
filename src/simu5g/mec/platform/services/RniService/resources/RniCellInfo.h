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
#include <map>
#include "simu5g/common/LteDefs.h"
#include "simu5g/mec/platform/services/resources/AttributeBase.h"
#include "simu5g/mec/platform/services/RniService/resources/Ecgi.h"
#include "simu5g/corenetwork/statsCollector/UeStatsCollector.h"
#include "simu5g/corenetwork/statsCollector/BaseStationStatsCollector.h"

namespace simu5g {

class RniCellInfo : public AttributeBase
{
  protected:
    opp_component_ptr<BaseStationStatsCollector> collector_; // it has the cellCollector and the map <Ipue -> uecollector>
    Ecgi ecgi_;

    nlohmann::ordered_json toJsonCell() const; // should this be private?

  public:
    RniCellInfo();
    RniCellInfo(BaseStationStatsCollector *eNodeB);

    UeStatsCollectorMap *getCollectorMap() const;

    Ecgi getEcgi() const;

    nlohmann::ordered_json toJson() const override;
};

} //namespace

#endif // _RNICELLINFO_H_

