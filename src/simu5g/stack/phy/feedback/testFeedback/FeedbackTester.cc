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

#include "stack/phy/feedback/testFeedback/FeedbackTester.h"

namespace simu5g {

Define_Module(FeedbackTester);

using namespace omnetpp;

void FeedbackTester::initialize()
{
    interval_ = par("interval");
    generator_.reference(this, "feedbackGeneratorModule", true);
    aperiodic_ = new cMessage("aperiodic");
    scheduleAt(simTime(), aperiodic_);
}

void FeedbackTester::handleMessage(cMessage *msg)
{
    if (msg == aperiodic_) {
        scheduleAt(simTime() + interval_, aperiodic_);
        generator_->aperiodicRequest();
    }
}

} //namespace

