#include "simu5g/stack/ip2nic/TechnologyDecision.h"

namespace simu5g {

Define_Module(TechnologyDecision);

void TechnologyDecision::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        lowerLayerOut_ = gate("lowerLayerOut");
    }
}

void TechnologyDecision::handleMessage(cMessage *msg)
{
    // Pass through for now
    send(msg, lowerLayerOut_);
}

} //namespace
