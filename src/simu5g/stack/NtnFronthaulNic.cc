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

#include "simu5g/stack/NtnFronthaulNic.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnFronthaulNic);

void NtnFronthaulNic::handleMessage(cMessage *msg)
{
    cGate *arrivalGate = msg->getArrivalGate();
    if (arrivalGate->isName("upperLayerIn")) {
        EV << "NtnFronthaulNic::handleMessage - forwarding packet " << msg->getName() << " to fronthaul link" << endl;
        send(msg, "phys$o");
    }
    else if (arrivalGate->isName("phys$i")) {
        EV << "NtnFronthaulNic::handleMessage - forwarding packet " << msg->getName() << " to upper layer" << endl;
        send(msg, "upperLayerOut");
    }
    else
        throw cRuntimeError("NtnFronthaulNic::handleMessage - unexpected gate %s", arrivalGate->getFullName());
}

} // namespace simu5g
