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

#include "simu5g/stack/pdcp/DcMux.h"

namespace simu5g {

Define_Module(DcMux);

void DcMux::initialize()
{
    dcManagerInGate_ = gate("dcManagerIn");
    fromLowerMuxGate_ = gate("fromLowerMux");
}

void DcMux::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming == dcManagerInGate_) {
        // Passthrough: forward DC manager input to LowerMux
        send(msg, "toLowerMux");
    }
    else if (incoming == fromLowerMuxGate_) {
        // Passthrough: forward LowerMux output to DC manager
        send(msg, "dcManagerOut");
    }
    else {
        throw cRuntimeError("DcMux: unexpected message from gate %s", incoming->getFullName());
    }
}

} // namespace simu5g
