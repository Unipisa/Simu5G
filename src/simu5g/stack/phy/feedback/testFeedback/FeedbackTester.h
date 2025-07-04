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

#ifndef _LTE_FEEDBACKTESTER_H_
#define _LTE_FEEDBACKTESTER_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteDefs.h"
#include "simu5g/stack/phy/feedback/LteDlFeedbackGenerator.h"

namespace simu5g {

using namespace omnetpp;

/**
 * TODO
 */
class FeedbackTester : public cSimpleModule
{
    simtime_t interval_;
    cMessage *aperiodic_ = nullptr;
    ModuleRefByPar<LteDlFeedbackGenerator> generator_;

  protected:
    void initialize() override;
    void handleMessage(cMessage *msg) override;
};

} //namespace

#endif

