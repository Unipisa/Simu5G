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

#ifndef _CARRIERASSIGNERBESTCHANNEL_H_
#define _CARRIERASSIGNERBESTCHANNEL_H_

#include "stack/mac/scheduler/CarrierAssigner.h"

/**
 * @class CarrierAssigner
 */
class CarrierAssignerBestChannel : public CarrierAssigner
{

  public:

    /**
     * Default constructor.
     */
    CarrierAssignerBestChannel(LteSchedulerEnb* schedEnb, LteMacEnb* mac, Direction dir) :
        CarrierAssigner(schedEnb, mac, dir)
    {
    }
    /**
     * Destructor.
     */
    virtual ~CarrierAssignerBestChannel()
    {
    }

    // run the carrier assignment algorithm
    virtual void assign();
};

#endif // _CARRIERASSIGNER_H_
