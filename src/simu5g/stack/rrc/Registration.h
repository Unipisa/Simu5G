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

#include "simu5g/common/LteCommon.h"
#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/NetworkInterface.h>

using namespace omnetpp;

namespace simu5g {

/**
 * @brief RRC Registration — registers the node with the Binder, sets up
 *        the network interface, and joins multicast groups.
 */
class Registration : public cSimpleModule
{
  private:
    MacNodeId lteNodeId = NODEID_NONE;
    MacNodeId nrNodeId = NODEID_NONE;
    RanNodeType nodeType = UNKNOWN_NODE_TYPE;

    // corresponding entry for our interface
    opp_component_ptr<inet::NetworkInterface> networkIf;

    inet::ModuleRefByPar<Binder> binder;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;
    void finish() override;

    virtual void registerInterface();
    virtual void registerMulticastGroups();

  public:
    RanNodeType getNodeType() const { return nodeType; }
    MacNodeId getLteNodeId() const { return lteNodeId; }
    MacNodeId getNrNodeId() const { return nrNodeId; }
    bool isDualTechnology() const { return lteNodeId != NODEID_NONE && nrNodeId != NODEID_NONE; }
};

} // namespace simu5g

#endif
