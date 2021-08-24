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



class RNICellInfo : public AttributeBase {
  protected:
    BaseStationStatsCollector* collector_; // it has the cellCollector and the map <Ipue -> uecollector>
    Ecgi ecgi_;

  /**
   * 
   * or std::map<ipv4, cellUeInfo>
   * I prefer the pointer to the list of users in the cell to manage better
//   * new/deleted users without the need of take care of them here
//   */
//    UeList* ueList_;
//    //Ecgi ecgi_;

    nlohmann::ordered_json toJsonCell() const; //should be private?
  /* data */


public:
  RNICellInfo();
  RNICellInfo(BaseStationStatsCollector* eNodeB);
  virtual ~RNICellInfo();

  UeStatsCollectorMap* getCollectorMap() const;
  
  Ecgi getEcgi() const;

  nlohmann::ordered_json toJson() const override;
//  nlohmann::ordered_json toJson(std::vector<Ipv4>& uesID) const;

};

#endif // _RNICELLINFO_H_

