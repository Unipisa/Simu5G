//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "../../RNIService/resources/Plmn.h"

Plmn::Plmn()
{
    // test mcc and mnc
    mcc_ = "001";
    mnc_ = "01";
}


Plmn::Plmn(const std::string& mcc, const std::string& mnc)
{
    setMcc(mcc);
    setMnc(mnc);
}

Plmn::~Plmn(){}

void Plmn::setMcc(const std::string& mcc)
{
    mcc_ = mcc;
}

void Plmn::setMnc(const std::string& mnc)
{
    mnc_ = mnc;
}

std::string Plmn::getMcc() const
{
    return mcc_;
}
std::string Plmn::getMnc() const
{
    return mnc_;
}

nlohmann::ordered_json Plmn::toJson() const
{
    nlohmann::ordered_json val;
    val["mcc"] = getMcc();
    val["mnc"] = getMnc();

    return val;

}


