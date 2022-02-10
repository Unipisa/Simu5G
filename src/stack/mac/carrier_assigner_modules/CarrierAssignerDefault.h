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

#ifndef _CARRIERASSIGNERDEFAULT_H_
#define _CARRIERASSIGNERDEFAULT_H_

#include "stack/mac/scheduler/CarrierAssigner.h"

/**
 * @class CarrierAssignerDefault
 */
class CarrierAssignerDefault : public CarrierAssigner
{

  public:

    /**
     * Default constructor.
     */
    CarrierAssignerDefault(LteSchedulerEnb* schedEnb, LteMacEnb* mac, Direction dir) :
        CarrierAssigner(schedEnb, mac, dir)
    {
    }
    /**
     * Destructor.
     */
    virtual ~CarrierAssignerDefault()
    {
    }

    // run the carrier assignment algorithm
    virtual void assign();
};

#endif // _CARRIERASSIGNERDEFAULT_H_
