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

#ifndef _CELLUEINFO_H_
#define _CELLUEINFO_H_

#include <vector>
#include <map>
#include "simu5g/common/LteDefs.h"
#include "simu5g/mec/platform/services/resources/AttributeBase.h"
#include "simu5g/mec/utils/MecCommon.h"
#include "simu5g/mec/platform/services/RniService/resources/Ecgi.h"
#include "AssociateId.h"

namespace simu5g {

class UeStatsCollector;

class CellUeInfo : public AttributeBase
{
  protected:
    UeStatsCollector *ueCollector_ = nullptr;
    AssociateId associateId_;
    Ecgi ecgi_;

    nlohmann::ordered_json toJsonCell() const;

  public:
    CellUeInfo();
    CellUeInfo(UeStatsCollector *ueCollector, const Ecgi& ecgi);
    CellUeInfo(UeStatsCollector *ueCollector, const mec::Ecgi& ecgi);


    nlohmann::ordered_json toJson() const override;
};

} //namespace

#endif

