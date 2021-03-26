#ifndef _PLMN_H_
#define _PLMN_H_

#include "nodes/mec/MecCommon.h"
#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"

// https://en.wikipedia.org/wiki/Mobile_country_code


class Plmn : public AttributeBase {
  protected:
    std::string mcc_; // 3 decimal digits
    std::string mnc_; // 2 or 3 decimal digits

    nlohmann::ordered_json toJsonCell() const; //should be private?
  /* data */


public:
  Plmn();
  Plmn(const std::string& mcc, const std::string& mnc);
  virtual ~Plmn();

    void setMcc(const std::string& mcc);
    void setMnc(const std::string& mnc);
    std::string getMcc() const;
    std::string getMnc() const;
    
    
  nlohmann::ordered_json toJson() const override;

};

#endif // _PLMN_H_

