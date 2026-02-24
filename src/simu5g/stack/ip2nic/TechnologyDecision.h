#ifndef __TECHNOLOGYDECISION_H_
#define __TECHNOLOGYDECISION_H_

#include <omnetpp.h>
#include <inet/common/InitStages.h>

namespace simu5g {

using namespace omnetpp;

class TechnologyDecision : public cSimpleModule
{
  protected:
    cGate *lowerLayerOut_ = nullptr;

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;
};

} //namespace

#endif
