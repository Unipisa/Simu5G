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

#include "common/binder/Binder.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/mobility/base/MovingMobilityBase.h"
#include "inet/mobility/contract/IMobility.h"



namespace LocationUtils{

    /* From: RESTful Network APIforTerminal Location
    * section 5.2.3.1
    */
    enum EnteringLeavingCriteria {Entering, Leaving};
    /* From: RESTful Network APIforTerminal Location
    * section 5.2.3.2
    */
    enum DistanceCriteria {AllWithinDistance, AnyWithinDistance, AllBeyondDistance, AnyBeyondDistance};

    inet::Coord getCoordinates(const MacNodeId id);
    inet::Coord getSpeed(const MacNodeId id);

}



#endif /* APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_LOCATIONAPIDEFS_H_ */
