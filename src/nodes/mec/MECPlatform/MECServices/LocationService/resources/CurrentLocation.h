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

#ifndef APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_CURRENTLOCATION_H_
#define APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_CURRENTLOCATION_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"

namespace simu5g {

class CurrentLocation : public AttributeBase
{
  protected:
    double accuracy;
    inet::Coord coords;
    TimeStamp timeStamp;

  public:
    CurrentLocation();
    CurrentLocation(double accuracy, const inet::Coord& coords, const TimeStamp& ts);
    CurrentLocation(double accuracy, const inet::Coord& coords);

    nlohmann::ordered_json toJson() const override;
};

} //namespace

#endif /* APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_CURRENTLOCATION_H_ */

