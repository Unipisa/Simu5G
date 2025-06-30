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

#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"

namespace simu5g {

class TimeStamp : public AttributeBase
{

  protected:
    int32_t seconds_;
    int32_t nanoSeconds_;

    bool valid_;

  public:
    TimeStamp();
    TimeStamp(bool valid);


    bool isValid() const;
    void setValid(bool valid);

    nlohmann::ordered_json toJson() const override;

    /// <summary>
    /// The seconds part of the time. Time is defined as Unix time since January 1, 1970, 00:00:00 UTC
    /// </summary>
    int32_t getSeconds() const;
    void setSeconds(int32_t value);
    void setSeconds();
    /// <summary>
    /// The nanoseconds part of the time. Time is defined as Unix time since January 1, 1970, 00:00:00 UTC
    /// </summary>
    int32_t getNanoSeconds() const;
    void setNanoSeconds(int32_t value);

};

} //namespace

#endif /* _TIMESTAMP_H_ */

