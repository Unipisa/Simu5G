#ifndef __SIMU5G_NTNFRONTHAULNIC_H_
#define __SIMU5G_NTNFRONTHAULNIC_H_

#include <omnetpp.h>

namespace simu5g {

class NtnFronthaulNic : public omnetpp::cSimpleModule
{
  protected:
    void handleMessage(omnetpp::cMessage *msg) override;
};

} // namespace simu5g

#endif
