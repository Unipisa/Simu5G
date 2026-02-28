#include "simu5g/stack/rlc/RlcUpperMux.h"

namespace simu5g {

Define_Module(RlcUpperMux);

void RlcUpperMux::initialize()
{
    upperLayerInGate_ = gate("upperLayerIn");
    upperLayerOutGate_ = gate("upperLayerOut");
    toUmGate_ = gate("toUm");
    fromUmGate_ = gate("fromUm");
}

void RlcUpperMux::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming == upperLayerInGate_) {
        send(msg, toUmGate_);
    }
    else if (incoming == fromUmGate_) {
        send(msg, upperLayerOutGate_);
    }
    else {
        throw cRuntimeError("RlcUpperMux: unexpected message from gate %s", incoming->getFullName());
    }
}

} // namespace simu5g
