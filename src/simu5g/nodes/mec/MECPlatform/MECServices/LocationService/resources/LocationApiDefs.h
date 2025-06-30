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

#ifndef APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_LOCATIONAPIDEFS_H_
#define APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_LOCATIONAPIDEFS_H_

#include <inet/common/geometry/common/Coord.h>
#include <inet/mobility/base/MovingMobilityBase.h>
#include <inet/mobility/contract/IMobility.h>

#include "common/binder/Binder.h"

namespace simu5g {

namespace LocationUtils {

/* From: RESTful Network API for Terminal Location
 * section 5.2.3.1
 */
enum EnteringLeavingCriteria { Entering, Leaving };
/* From: RESTful Network API for Terminal Location
 * section 5.2.3.2
 */
enum DistanceCriteria { AllWithinDistance, AnyWithinDistance, AllBeyondDistance, AnyBeyondDistance };

inet::Coord getCoordinates(Binder *binder, const MacNodeId id);
inet::Coord getSpeed(Binder *binder, const MacNodeId id);

} // namespace LocationUtils

} //namespace

#endif /* APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_LOCATIONAPIDEFS_H_ */

