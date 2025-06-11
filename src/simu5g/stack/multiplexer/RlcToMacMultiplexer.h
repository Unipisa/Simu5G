//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __SIMU5G_RLCTOMACMULTIPLEXER_H_
#define __SIMU5G_RLCTOMACMULTIPLEXER_H_

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"

using namespace omnetpp;

namespace simu5g {

/**
 * TODO - Generated class
 */
class RlcToMacMultiplexer : public cSimpleModule
{
  protected:
    int numDrbs;

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    // Scheduling function stub (to be expanded later)
    int selectNextDrb();
};

} //namespace

#endif
