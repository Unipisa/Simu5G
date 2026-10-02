//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __SIMU5G_CELLULARNICNDSINK_H_
#define __SIMU5G_CELLULARNICNDSINK_H_

#include <omnetpp.h>

namespace simu5g {

using namespace omnetpp;

/**
 * WORKAROUND: discards the IPv6 Neighbor Discovery messages a base station's
 * network layer sends to its cellular NIC, and throws on anything else.
 * See the NED documentation.
 */
class CellularNicNdSink : public cSimpleModule
{
  protected:
    void handleMessage(cMessage *msg) override;
};

} // namespace simu5g

#endif
