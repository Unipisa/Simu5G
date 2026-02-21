//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __IP2NIC_H_
#define __IP2NIC_H_

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include <unordered_map>
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

class LteHandoverManager;
class LtePdcpBase;


/**
 *
 */
class Ip2Nic : public cSimpleModule
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
            // Combine hashes using XOR and bit shifting for better distribution
            return std::hash<uint32_t>()(key.srcAddr) ^ (std::hash<uint32_t>()(key.dstAddr) << 1) ^ (std::hash<uint16_t>()(key.typeOfService) << 2);
        }
    };

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

    // Flags mirroring PDCP's (to be verified with ASSERTs, then used to replace PDCP dependency)
    bool isNR_ = false;
    bool hasD2DSupport_ = false;
    LteRlcType conversationalRlc_ = UNKNOWN_RLC_TYPE;
    LteRlcType streamingRlc_ = UNKNOWN_RLC_TYPE;
    LteRlcType interactiveRlc_ = UNKNOWN_RLC_TYPE;
    LteRlcType backgroundRlc_ = UNKNOWN_RLC_TYPE;

    // for each connection using Split Bearer, keeps track of the number of packets sent down to the PDCP
    std::unordered_map<FlowKey, int, FlowKeyHash> splitBearersTable_;

    cGate *stackGateOut_ = nullptr;       // gate connecting Ip2Nic module to cellular stack
    cGate *ipGateOut_ = nullptr;          // gate connecting Ip2Nic module to network layer

    LtePdcpBase *pdcp_ = nullptr;         // sibling PDCP module (for calling analyzePacket)

    // corresponding entry for our interface
    opp_component_ptr<inet::NetworkInterface> networkIf;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

    virtual void prepareForIpv4(inet::Packet *datagram, const inet::Protocol *protocol = & inet::Protocol::ipv4);
    virtual void toIpUe(inet::Packet *datagram);
    virtual void toIpBs(inet::Packet *datagram);
    virtual void toStackBs(inet::Packet *datagram);
    virtual void toStackUe(inet::Packet *datagram);

    void printControlInfo(inet::Packet *pkt);

    // mark packet for using LTE, NR or split bearer
    //
    // In the current version, the Ip2Nic module of the master eNB (the UE) selects which path
    // to follow based on the Type of Service (TOS) field:
    // - use master eNB if tos < 10
    // - use secondary gNB if 10 <= tos < 20
    // - use split bearer if tos >= 20
    //
    // To change the policy, change the implementation of the Ip2Nic::markPacket() function
    //
    // TODO use a better policy
    bool markPacket(inet::Ipv4Address srcAddr, inet::Ipv4Address dstAddr, uint16_t typeOfService, bool& useNR);

  public:
    ~Ip2Nic() override;
};

} //namespace

#endif
