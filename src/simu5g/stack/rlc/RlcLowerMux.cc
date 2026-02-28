#include "simu5g/stack/rlc/RlcLowerMux.h"

namespace simu5g {

Define_Module(RlcLowerMux);

void RlcLowerMux::initialize()
{
    macInGate_ = gate("macIn");
    macOutGate_ = gate("macOut");
    toUmGate_ = gate("toUm");
    fromUmGate_ = gate("fromUm");
}

void RlcLowerMux::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming == macInGate_) {
        send(msg, toUmGate_);
    }
    else if (incoming == fromUmGate_) {
        send(msg, macOutGate_);
    }
    else {
        throw cRuntimeError("RlcLowerMux: unexpected message from gate %s", incoming->getFullName());
    }
}

} // namespace simu5g
