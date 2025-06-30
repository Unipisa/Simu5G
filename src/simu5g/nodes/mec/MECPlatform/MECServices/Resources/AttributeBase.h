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

#ifndef _ATTRIBUTEBASE_H_
#define _ATTRIBUTEBASE_H_

#include <ctime>
#include <string>

#include "nodes/mec/utils/httpUtils/json.hpp"
#include "common/LteCommon.h"

namespace simu5g {

class AttributeBase
{
  public:
    virtual nlohmann::ordered_json toJson() const = 0;
    static std::string toJson(const std::string& value);
    static std::string toJson(const std::time_t& value);
    static int32_t toJson(int32_t value);
    static int64_t toJson(int64_t value);
    static double toJson(double value);
    static bool toJson(bool value);
    static nlohmann::ordered_json toJson(AttributeBase& content);
    virtual ~AttributeBase() {}
};

} //namespace

#endif // _ATTRIBUTEBASE_H_

