#ifndef __TECHNOLOGYDECISION_H_
#define __TECHNOLOGYDECISION_H_

#include <omnetpp.h>
#include <unordered_map>
#include <inet/common/InitStages.h>
#include <inet/common/ModuleRefByPar.h>
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

class TechnologyDecision : public cSimpleModule
{
  protected:
    // Represents a flow using source address, destination address, and type of service.
    struct FlowKey {
        uint32_t srcAddr;
        uint32_t dstAddr;
        uint16_t typeOfService;
        bool operator==(const FlowKey& other) const { return srcAddr == other.srcAddr && dstAddr == other.dstAddr && typeOfService == other.typeOfService; }
    };

    // Hash function for FlowKey
    struct FlowKeyHash {
        std::size_t operator()(const FlowKey& key) const {
            return std::hash<uint32_t>()(key.srcAddr) ^ (std::hash<uint32_t>()(key.dstAddr) << 1) ^ (std::hash<uint16_t>()(key.typeOfService) << 2);
        }
    };

    cGate *lowerLayerOut_ = nullptr;

    RanNodeType nodeType_;      // UE or NODEB

    // reference to the binder
    inet::ModuleRefByPar<Binder> binder_;

    // LTE MAC node id of this node
    MacNodeId nodeId_ = NODEID_NONE;
    // NR MAC node id of this node (if enabled)
    MacNodeId nrNodeId_ = NODEID_NONE;

    // LTE MAC node id of this node's master
    MacNodeId servingNodeId_ = NODEID_NONE;
    // NR MAC node id of this node's master (if enabled)
    MacNodeId nrServingNodeId_ = NODEID_NONE;

    // Enable for dual connectivity
    bool dualConnectivityEnabled_;

    // for each connection using Split Bearer, keeps track of the number of packets sent down to the PDCP
    std::unordered_map<FlowKey, int, FlowKeyHash> splitBearersTable_;

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

  public:
    // mark packet for using LTE, NR or split bearer
    bool markPacket(inet::Ipv4Address srcAddr, inet::Ipv4Address dstAddr, uint16_t typeOfService, bool& useNR);
};

} //namespace

#endif
