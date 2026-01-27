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

#ifndef _RRC_H_
#define _RRC_H_

#include "simu5g/common/LteDefs.h"
#include "simu5g/common/LteControlInfo.h"
#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/NetworkInterface.h>

using namespace omnetpp;

namespace simu5g {

class LteMacBase;
class LtePdcpBase;
class LteRlcUm;

/**
 * @brief RRC (Radio Resource Control) module for LTE/NR networks.
 */
class Rrc : public cSimpleModule
{
  private:
    MacNodeId lteNodeId = NODEID_NONE;
    MacNodeId nrNodeId = NODEID_NONE;
    RanNodeType nodeType = UNKNOWN_NODE_TYPE;

    // corresponding entry for our interface
    opp_component_ptr<inet::NetworkInterface> networkIf;

    inet::ModuleRefByPar<Binder> binder;
    inet::ModuleRefByPar<LtePdcpBase> pdcpModule;
    inet::ModuleRefByPar<LteRlcUm> rlcUmModule;  // Compound module with TM/UM/AM submodules
    inet::ModuleRefByPar<LteRlcUm> nrRlcUmModule;  // same
    inet::ModuleRefByPar<LteMacBase> macModule;
    inet::ModuleRefByPar<LteMacBase> nrMacModule;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

    virtual void registerInterface();
    virtual void registerMulticastGroups();

  public:
    RanNodeType getNodeType() const { return nodeType; }
    MacNodeId getLteNodeId() const { return lteNodeId; }
    MacNodeId getNrNodeId() const { return nrNodeId; }
    bool isDualTechnology() const { return lteNodeId != NODEID_NONE && nrNodeId != NODEID_NONE; }

    virtual void createIncomingConnection(FlowControlInfo *lteInfo, bool withPdcp=true);
    virtual void createOutgoingConnection(FlowControlInfo *lteInfo, bool withPdcp=true);
};

} // namespace simu5g

#endif
