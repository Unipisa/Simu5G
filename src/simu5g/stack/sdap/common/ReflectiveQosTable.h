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

#ifndef STACK_SDAP_COMMON_REFLECTIVEQOSTABLE_H_
#define STACK_SDAP_COMMON_REFLECTIVEQOSTABLE_H_

#include <omnetpp.h>
#include <map>
#include <set>
#include <string>
#include <inet/common/packet/Packet.h>

using namespace omnetpp;

namespace simu5g {

// Structure to represent a flow key (5-tuple)
struct FlowKey {
    std::string srcAddr;
    std::string dstAddr;
    uint16_t srcPort = 0;
    uint16_t dstPort = 0;
    uint8_t protocol = 0;

    // Constructor
    FlowKey() = default;
    FlowKey(const std::string& src, const std::string& dst, uint16_t sp, uint16_t dp, uint8_t prot)
        : srcAddr(src), dstAddr(dst), srcPort(sp), dstPort(dp), protocol(prot) {}

    // Generate string representation for map keys
    std::string toString() const {
        return srcAddr + ":" + std::to_string(srcPort) + "->" +
               dstAddr + ":" + std::to_string(dstPort) + "/" + std::to_string(protocol);
    }

    // Generate reverse flow key (swap src/dst)
    FlowKey reverse() const {
        return FlowKey(dstAddr, srcAddr, dstPort, srcPort, protocol);
    }

    // Comparison operators for use in maps
    bool operator<(const FlowKey& other) const {
        if (srcAddr != other.srcAddr) return srcAddr < other.srcAddr;
        if (dstAddr != other.dstAddr) return dstAddr < other.dstAddr;
        if (srcPort != other.srcPort) return srcPort < other.srcPort;
        if (dstPort != other.dstPort) return dstPort < other.dstPort;
        return protocol < other.protocol;
    }

    bool operator==(const FlowKey& other) const {
        return srcAddr == other.srcAddr && dstAddr == other.dstAddr &&
               srcPort == other.srcPort && dstPort == other.dstPort &&
               protocol == other.protocol;
    }
};

// Structure to store reflective QoS flow information
struct ReflectiveQosFlow {
    uint8_t qfi = 0;
    FlowKey flowKey;
    simtime_t lastSeen;
    bool isActive = true;

    // Constructor
    ReflectiveQosFlow() = default;
    ReflectiveQosFlow(uint8_t q, const FlowKey& key)
        : qfi(q), flowKey(key), lastSeen(simTime()), isActive(true) {}
};

/**
 * ReflectiveQosTable maintains a table of QoS flow mappings for reflective QoS.
 *
 * This OMNeT++ module implements 3GPP TS 37.324 reflective QoS feature, which allows
 * UEs to derive uplink QoS flow characteristics from downlink traffic patterns.
 *
 * Key features:
 * - Tracks downlink flows with QFI associations
 * - Enables dynamic uplink QFI derivation
 * - Automatic flow expiration and cleanup
 * - Support for TCP and UDP protocols
 * - Configurable via module parameters
 */
class ReflectiveQosTable : public cSimpleModule
{
  private:
    // Configuration (from module parameters)
    simtime_t timeout_ = 30;

    std::map<FlowKey, ReflectiveQosFlow> reflectiveFlows_;
    cMessage *cleanupTimer_ = nullptr;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

  public:
    // Constructor and destructor
    ReflectiveQosTable();
    virtual ~ReflectiveQosTable();

    // Core reflective QoS functionality
    void handleDownlinkFlow(inet::Packet *pkt, uint8_t qfi);
    uint8_t lookupUplinkQfi(inet::Packet *pkt);

    // Flow management
    void cleanupExpiredFlows();
    size_t getActiveFlowCount() const { return reflectiveFlows_.size(); }
    void clearAllFlows();

    // Debug and monitoring
    void printFlows() const;

  private:
    // Helper methods
    FlowKey extractFlowKey(inet::Packet *pkt) const;

    // Timer management
    void scheduleCleanupTimer();
    void cancelCleanupTimer();
};

} // namespace simu5g

#endif /* STACK_SDAP_COMMON_REFLECTIVEQOSTABLE_H_ */
