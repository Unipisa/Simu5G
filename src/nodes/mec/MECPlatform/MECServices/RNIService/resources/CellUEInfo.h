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
#include <omnetpp.h>
#include <map>
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/utils/MecCommon.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/Ecgi.h"
#include "AssociateId.h"

namespace simu5g {

class UeStatsCollector;

class CellUEInfo : public AttributeBase
{
  protected:
    UeStatsCollector *ueCollector_ = nullptr;
    AssociateId associateId_;
    Ecgi ecgi_;

    nlohmann::ordered_json toJsonCell() const;

  public:
    CellUEInfo();
    CellUEInfo(UeStatsCollector *ueCollector, const Ecgi& ecgi);
    CellUEInfo(UeStatsCollector *ueCollector, const mec::Ecgi& ecgi);


    nlohmann::ordered_json toJson() const override;
};

} //namespace

#endif

