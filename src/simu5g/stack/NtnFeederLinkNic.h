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

#ifndef __SIMU5G_NTNFEEDERLINKNIC_H_
#define __SIMU5G_NTNFEEDERLINKNIC_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/InitStages.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

class NtnFeederLinkNic : public omnetpp::cSimpleModule
{
  protected:
    inet::ModuleRefByPar<Binder> binder_;
    MacNodeId nodeId_ = NODEID_NONE;
    RanNodeType nodeType_ = UNKNOWN_NODE_TYPE;

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(omnetpp::cMessage *msg) override;

    omnetpp::cGate *resolvePeerGate() const;
    omnetpp::cModule *resolvePeerNode() const;
};

} // namespace simu5g

#endif
