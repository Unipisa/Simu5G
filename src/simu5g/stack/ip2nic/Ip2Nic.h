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

#ifndef __IP2NIC_H_
#define __IP2NIC_H_

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include <set>
#include <unordered_map>
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

class LteHandoverManager;


/**
 *
 */
class Ip2Nic : public cSimpleModule
{
  protected:
    RanNodeType nodeType_;      // UE or NODEB

    // reference to the binder
    inet::ModuleRefByPar<Binder> binder_;

    // LTE MAC node id of this node
    MacNodeId nodeId_ = NODEID_NONE;
    // NR MAC node id of this node (if enabled)
    MacNodeId nrNodeId_ = NODEID_NONE;

    // Enable for dual connectivity
    bool dualConnectivityEnabled_;

    // Flags mirroring PDCP's (to be verified with ASSERTs, then used to replace PDCP dependency)
    bool isNR_ = false;
    bool hasD2DSupport_ = false;
    LteRlcType conversationalRlc_ = UNKNOWN_RLC_TYPE;
    LteRlcType streamingRlc_ = UNKNOWN_RLC_TYPE;
    LteRlcType interactiveRlc_ = UNKNOWN_RLC_TYPE;
    LteRlcType backgroundRlc_ = UNKNOWN_RLC_TYPE;

    // Key for identifying connections (for DRB ID assignment)
    struct ConnectionKey {
        inet::Ipv4Address srcAddr;
        inet::Ipv4Address dstAddr;
        uint16_t typeOfService;
        Direction direction;

        bool operator==(const ConnectionKey& other) const {
            return srcAddr == other.srcAddr &&
                   dstAddr == other.dstAddr &&
                   typeOfService == other.typeOfService &&
                   direction == other.direction;
        }
    };

    struct ConnectionKeyHash {
        std::size_t operator()(const ConnectionKey& key) const {
            std::size_t h1 = std::hash<uint32_t>{}(key.srcAddr.getInt());
            std::size_t h2 = std::hash<uint32_t>{}(key.dstAddr.getInt());
            std::size_t h3 = std::hash<uint16_t>{}(key.typeOfService);
            std::size_t h4 = std::hash<uint16_t>{}(uint16_t(key.direction));
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };

    // DRB ID counter and table (for DRB ID assignment)
    unsigned short drbId_ = 1;
    std::unordered_map<ConnectionKey, DrbId, ConnectionKeyHash> drbIdTable_;

    // Tracks (drbId, destId) pairs for which connections have been established.
    // A new entry triggers establishUnidirectionalDataConnection(); this also
    // handles re-establishment after handover (same drbId, different destId).
    std::set<std::pair<DrbId, MacNodeId>> establishedConnections_;

    cGate *stackGateOut_ = nullptr;       // gate connecting Ip2Nic module to cellular stack
    cGate *ipGateOut_ = nullptr;          // gate connecting Ip2Nic module to network layer

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

    // Packet analysis (moved from PDCP): classifies the packet and fills FlowControlInfo tag
    void analyzePacket(inet::Packet *pkt, inet::Ipv4Address srcAddr, inet::Ipv4Address destAddr, uint16_t typeOfService);
    MacNodeId getNextHopNodeId(const inet::Ipv4Address& destAddr, bool useNR, MacNodeId sourceId);
    LteTrafficClass getTrafficCategory(cPacket *pkt);
    LteRlcType getRlcType(LteTrafficClass trafficCategory);
    DrbId lookupOrAssignDrbId(const ConnectionKey& key);

  public:
    ~Ip2Nic() override;
};

} //namespace

#endif
