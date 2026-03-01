#include "simu5g/stack/rlc/tm/TmRxEntity.h"

namespace simu5g {

Define_Module(TmRxEntity);

using namespace omnetpp;

void TmRxEntity::initialize(int stage)
{
    // nothing to initialize
}

void TmRxEntity::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming->isName("in")) {
        EV << "TmRxEntity::handleMessage - forwarding packet " << msg->getName() << " to upper layer\n";
        send(msg, "out");
    }
    else {
        throw cRuntimeError("TmRxEntity: unexpected message from gate %s", incoming->getFullName());
    }
}

} // namespace simu5g
