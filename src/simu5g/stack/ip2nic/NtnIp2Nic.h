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

#ifndef __SIMU5G_NTNIP2NIC_H_
#define __SIMU5G_NTNIP2NIC_H_

#include "simu5g/stack/ip2nic/Ip2Nic.h"

namespace simu5g {

class NtnIp2Nic : public Ip2Nic
{
  protected:
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override;
    bool registerNtnAssociation(bool throwOnMissing);
};

} // namespace simu5g

#endif
