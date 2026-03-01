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

void DcMux::handleMessage(cMessage *msg)
{
    throw cRuntimeError("DcMux: not yet implemented");
}

} // namespace simu5g
