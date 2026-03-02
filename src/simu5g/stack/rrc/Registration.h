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

#ifndef _REGISTRATION_H_
#define _REGISTRATION_H_

#include <omnetpp.h>

using namespace omnetpp;

namespace simu5g {

/**
 * @brief RRC Registration — registers the node with the Binder, sets up
 *        the network interface, and joins multicast groups.
 */
class Registration : public cSimpleModule
{
  protected:
    void initialize(int stage) override;
    int numInitStages() const override;
    void handleMessage(cMessage *msg) override;
};

} // namespace simu5g

#endif
