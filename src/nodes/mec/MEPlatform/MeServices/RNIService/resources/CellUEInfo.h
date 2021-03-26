#ifndef _CELLUEINFO_H_
#define _CELLUEINFO_H_


#include <vector>
#include <omnetpp.h>
#include <map>
#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"
#include "nodes/mec/MecCommon.h"
#include "../../RNIService/resources/Ecgi.h"
#include "AssociateId.h"

class UeStatsCollector;

class CellUEInfo : public AttributeBase {
  protected:
    UeStatsCollector *ueCollector_;
    AssociateId associateId_;
    Ecgi ecgi_;

  /**
   * 
   * or std::map<ipv4, cellUeInfo>
   * I prefer the pointer to the list of users in the cell to manage better
//   * new/deleted users without the need of take care of them here
//   */
//    UeList* ueList_;
//    //Ecgi ecgi_;

    nlohmann::ordered_json toJsonCell() const;


public:
  CellUEInfo();
  CellUEInfo(UeStatsCollector* ueCollector, const Ecgi& ecgi);
  CellUEInfo(UeStatsCollector* ueCollector, const mec::Ecgi& ecgi);

  virtual ~CellUEInfo();

  nlohmann::ordered_json toJson() const override;
//  nlohmann::ordered_json toJson(std::vector<Ipv4>& uesID) const;

};








#endif
