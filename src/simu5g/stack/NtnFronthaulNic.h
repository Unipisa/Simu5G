#ifndef __SIMU5G_NTNFRONTHAULNIC_H_
#define __SIMU5G_NTNFRONTHAULNIC_H_

#include <omnetpp.h>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

class NtnFronthaulNic : public omnetpp::cSimpleModule
{
  protected:
    GHz feederLinkFrequencyOffset_ = GHz(NTN_FEEDER_LINK_FREQUENCY_OFFSET_GHZ);
    bool shiftFrequencyFromFeederLink_ = false;

    void initialize() override;
    void handleMessage(omnetpp::cMessage *msg) override;
    void shiftFrequencyBandToServiceLink(omnetpp::cMessage *msg) const;
};

} // namespace simu5g

#endif
