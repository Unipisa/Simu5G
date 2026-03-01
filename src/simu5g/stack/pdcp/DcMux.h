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

#ifndef _PDCP_DC_MUX_H_
#define _PDCP_DC_MUX_H_

#include <omnetpp.h>

namespace simu5g {

using namespace omnetpp;

class DcMux : public cSimpleModule
{
  protected:
    void handleMessage(cMessage *msg) override;
};

} // namespace simu5g

#endif
