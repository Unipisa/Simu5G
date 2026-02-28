#ifndef _RLC_UPPER_MUX_H_
#define _RLC_UPPER_MUX_H_

#include <omnetpp.h>

namespace simu5g {

using namespace omnetpp;

/**
 * @class RlcUpperMux
 * @brief Upper-layer RLC packet dispatcher (pass-through for now).
 */
class RlcUpperMux : public cSimpleModule
{
  protected:
    cGate *upperLayerInGate_ = nullptr;
    cGate *upperLayerOutGate_ = nullptr;
    cGate *toUmGate_ = nullptr;
    cGate *fromUmGate_ = nullptr;

  protected:
    void initialize() override;
    void handleMessage(cMessage *msg) override;
};

} // namespace simu5g

#endif
