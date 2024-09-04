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

#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/Plmn.h"

namespace simu5g {

Plmn::Plmn() : mcc_("001"), mnc_("01") // test mcc and mnc
{
}

Plmn::Plmn(const std::string& mcc, const std::string& mnc)
{
    setMcc(mcc);
    setMnc(mnc);
}


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

} //namespace

