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

#ifndef __PLATOONCONTROLLERSAMPLE_H_
#define __PLATOONCONTROLLERSAMPLE_H_

#include "omnetpp.h"
#include "apps/mec/PlatooningApp/platoonController/PlatoonControllerBase.h"

using namespace std;
using namespace omnetpp;

class MECPlatooningApp;

/*
 * PlatoonControllerSample
 *
 * This class implements a sample platoon controller
 */
class PlatoonControllerSample : public PlatoonControllerBase
{

  protected:

    // @brief run the global platoon controller
    virtual const CommandList* controlPlatoon();

  public:
    PlatoonControllerSample();
    PlatoonControllerSample(MECPlatooningProviderApp* mecPlatooningProviderApp, int index, double controlPeriod = 1.0);
    virtual ~PlatoonControllerSample();
};

#endif
