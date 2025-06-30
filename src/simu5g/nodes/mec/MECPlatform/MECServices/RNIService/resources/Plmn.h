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

#ifndef _PLMN_H_
#define _PLMN_H_

#include "nodes/mec/utils/MecCommon.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"

namespace simu5g {

// https://en.wikipedia.org/wiki/Mobile_country_code

class Plmn : public AttributeBase
{
  protected:
    std::string mcc_; // 3 decimal digits
    std::string mnc_; // 2 or 3 decimal digits

    nlohmann::ordered_json toJsonCell() const; // should this be private?
    /* data */

  public:
    Plmn();
    Plmn(const std::string& mcc, const std::string& mnc);

    void setMcc(const std::string& mcc);
    void setMnc(const std::string& mnc);
    std::string getMcc() const;
    std::string getMnc() const;

    nlohmann::ordered_json toJson() const override;

};

} //namespace

#endif // _PLMN_H_

