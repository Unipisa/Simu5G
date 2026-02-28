#ifndef _RLC_LOWER_MUX_H_
#define _RLC_LOWER_MUX_H_

#include <omnetpp.h>

namespace simu5g {

using namespace omnetpp;

/**
 * @class RlcLowerMux
 * @brief Lower-layer RLC packet dispatcher (pass-through for now).
 */
class RlcLowerMux : public cSimpleModule
{
  protected:
    cGate *macInGate_ = nullptr;
    cGate *macOutGate_ = nullptr;
    cGate *toUmGate_ = nullptr;
    cGate *fromUmGate_ = nullptr;

  protected:
    void initialize() override;
    void handleMessage(cMessage *msg) override;
};

} // namespace simu5g

#endif
