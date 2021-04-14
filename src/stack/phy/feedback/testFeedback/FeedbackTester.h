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

#include <omnetpp.h>
#include "stack/phy/feedback/LteDlFeedbackGenerator.h"

/**
 * TODO
 */
class FeedbackTester : public omnetpp::cSimpleModule
{
    omnetpp::simtime_t interval_;
    omnetpp::cMessage *aperiodic_;
    LteDlFeedbackGenerator *generator_;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
};

#endif
