#ifndef _ECGI_H_
#define _ECGI_H_

#include <string>

#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"
#include "nodes/mec/MEPlatform/MeServices/RNIService/resources/Plmn.h"


class Ecgi : public AttributeBase {
  protected:
    MacCellId cellId_;
    Plmn plmn_;

    nlohmann::ordered_json toJsonCell() const; //should be private?

public:
  Ecgi();
  Ecgi(MacCellId cellId);
  Ecgi(const mec::Ecgi ecgi);

  Ecgi(MacCellId cellId, Plmn& plmn);
  
  virtual ~Ecgi();
  
  void setCellId(const MacCellId cellId);
  void setPlmn(const Plmn& plmn);
  void setPlmn(const mec::Plmn plmn);
  void setEcgi(const mec::Ecgi& ecgi);

  MacCellId getCellId() const;
  Plmn getPlmn() const;

  nlohmann::ordered_json toJson() const override;

};

#endif // _ECGI_H_

