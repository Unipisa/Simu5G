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

#include "ReflectiveQosTable.h"
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>
#include <iostream>
#include <sstream>

using namespace simu5g;

Define_Module(ReflectiveQosTable);

ReflectiveQosTable::ReflectiveQosTable()
{
}

ReflectiveQosTable::~ReflectiveQosTable()
{
    cancelCleanupTimer();
    clearAllFlows();
}

void ReflectiveQosTable::initialize()
{
    // Read module parameters
    timeout_ = par("timeout").doubleValue();

    // Initialize cleanup timer
    cleanupTimer_ = new cMessage("cleanupTimer");
    scheduleCleanupTimer();
}

void ReflectiveQosTable::handleMessage(cMessage *msg)
{
    if (msg == cleanupTimer_) {
        // Periodic cleanup of expired flows
        cleanupExpiredFlows();
        scheduleCleanupTimer();
    }
    else {
        throw cRuntimeError(this, "Unknown message received");
    }
}

void ReflectiveQosTable::finish()
{
    EV_INFO << "ReflectiveQosTable: Final statistics - Active flows: " << reflectiveFlows_.size() << "\n";
    if (par("printFinalStats").boolValue())
        printFlows();
}


void ReflectiveQosTable::handleDownlinkFlow(inet::Packet *pkt, uint8_t qfi)
{
    // Extract flow key from the packet
    FlowKey downlinkFlowKey = extractFlowKey(pkt);

    if (downlinkFlowKey.srcAddr.empty() || downlinkFlowKey.dstAddr.empty()) {
        EV_WARN << "ReflectiveQosTable: Could not extract flow information for reflective QoS\n";
        return;
    }

    // Generate reverse flow key for the uplink direction (UE perspective)
    FlowKey uplinkFlowKey = downlinkFlowKey.reverse();

    // Store or update the reflective flow
    auto it = reflectiveFlows_.find(uplinkFlowKey);
    if (it != reflectiveFlows_.end()) {
        // Update existing flow
        it->second.lastSeen = simTime();
        it->second.isActive = true;
        EV_INFO << "ReflectiveQosTable: Updated reflective QoS flow: " << uplinkFlowKey.toString()
                << " -> QFI " << (int)qfi << "\n";
    } else {
        // Create new reflective flow
        ReflectiveQosFlow newFlow(qfi, uplinkFlowKey);
        reflectiveFlows_[uplinkFlowKey] = newFlow;
        EV_INFO << "ReflectiveQosTable: Created new reflective QoS flow: " << uplinkFlowKey.toString()
                << " -> QFI " << (int)qfi << "\n";
    }

    // Cleanup expired flows periodically
    cleanupExpiredFlows();
}

uint8_t ReflectiveQosTable::lookupUplinkQfi(inet::Packet *pkt)
{
    if (reflectiveFlows_.empty())
        return 0;

    // Extract flow key from the packet
    FlowKey uplinkFlowKey = extractFlowKey(pkt);

    if (uplinkFlowKey.srcAddr.empty() || uplinkFlowKey.dstAddr.empty())
        return 0;

    // Look up the flow in reflective flows
    auto it = reflectiveFlows_.find(uplinkFlowKey);
    if (it != reflectiveFlows_.end() && it->second.isActive) {
        // Check if flow hasn't expired
        if (simTime() - it->second.lastSeen <= timeout_) {
            EV_INFO << "ReflectiveQosTable: Found reflective QoS match: " << uplinkFlowKey.toString()
                    << " -> QFI " << (int)it->second.qfi << "\n";
            return it->second.qfi;
        }
        else {
            // Flow expired, mark as inactive
            it->second.isActive = false;
            EV_INFO << "ReflectiveQosTable: Reflective QoS flow expired: " << uplinkFlowKey.toString() << "\n";
        }
    }

    return 0; // No match found
}

void ReflectiveQosTable::cleanupExpiredFlows()
{
    if (reflectiveFlows_.empty())
        return;

    simtime_t currentTime = simTime();
    auto it = reflectiveFlows_.begin();
    int removedCount = 0;

    while (it != reflectiveFlows_.end()) {
        if (currentTime - it->second.lastSeen > timeout_) {
            EV_INFO << "ReflectiveQosTable: Removing expired reflective QoS flow: " << it->first.toString() << "\n";
            it = reflectiveFlows_.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }

    if (removedCount > 0) {
        EV_INFO << "ReflectiveQosTable: Cleaned up " << removedCount << " expired reflective QoS flows\n";
    }
}

void ReflectiveQosTable::clearAllFlows()
{
    reflectiveFlows_.clear();
    EV_INFO << "ReflectiveQosTable: Cleared all reflective QoS flows\n";
}

void ReflectiveQosTable::printFlows() const
{
    EV_INFO << "=== ReflectiveQosTable Status ===" << std::endl;
    EV_INFO << "Timeout: " << timeout_ << "s" << std::endl;
    EV_INFO  << "Active flows: " << reflectiveFlows_.size() << std::endl;

    if (!reflectiveFlows_.empty()) {
        std::cout << "Flow mappings:" << std::endl;
        for (const auto& pair : reflectiveFlows_) {
            const ReflectiveQosFlow& flow = pair.second;
            EV_INFO << "  " << pair.first.toString() << " -> QFI " << (int)flow.qfi
                     << " (last seen: " << flow.lastSeen << ", active: " << (flow.isActive ? "Yes" : "No") << ")" << std::endl;
        }
    }
    EV_INFO << "=================================" << std::endl;
}

FlowKey ReflectiveQosTable::extractFlowKey(inet::Packet *pkt) const
{
    FlowKey flowKey;

    try {
        // Extract IP header
        auto ipHeader = pkt->peekAtFront<inet::Ipv4Header>();
        if (!ipHeader) {
            return flowKey; // Return empty FlowKey
        }

        flowKey.srcAddr = ipHeader->getSrcAddress().str();
        flowKey.dstAddr = ipHeader->getDestAddress().str();
        flowKey.protocol = ipHeader->getProtocolId();

        // Extract transport layer ports
        auto ipHeaderLength = ipHeader->getHeaderLength();

        if (flowKey.protocol == inet::IP_PROT_TCP) {
            auto tcpHeader = pkt->peekDataAt<inet::tcp::TcpHeader>(ipHeaderLength);
            if (tcpHeader) {
                flowKey.srcPort = tcpHeader->getSrcPort();
                flowKey.dstPort = tcpHeader->getDestPort();
            }
        } else if (flowKey.protocol == inet::IP_PROT_UDP) {
            auto udpHeader = pkt->peekDataAt<inet::UdpHeader>(ipHeaderLength);
            if (udpHeader) {
                flowKey.srcPort = udpHeader->getSrcPort();
                flowKey.dstPort = udpHeader->getDestPort();
            }
        }
    } catch (const std::exception& e) {
        EV_WARN << "ReflectiveQosTable: Error extracting flow info: " << e.what() << "\n";
        // Return empty FlowKey on error
        flowKey = FlowKey();
    }

    return flowKey;
}

void ReflectiveQosTable::scheduleCleanupTimer()
{
    if (cleanupTimer_ != nullptr) {
        simtime_t cleanupInterval = par("cleanupInterval").doubleValue();
        scheduleAt(simTime() + cleanupInterval, cleanupTimer_);
    }
}

void ReflectiveQosTable::cancelCleanupTimer()
{
    if (cleanupTimer_ != nullptr) {
        cancelAndDelete(cleanupTimer_);
        cleanupTimer_ = nullptr;
    }
}
