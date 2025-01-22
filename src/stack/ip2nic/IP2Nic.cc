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
#include <iostream>
#include <inet/common/ModuleAccess.h>
#include <inet/common/IInterfaceRegistrationListener.h>
#include <inet/common/socket/SocketTag_m.h>

#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/Udp.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>
#include <inet/transportlayer/common/L4Tools.h>

#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>
#include <inet/networklayer/ipv4/Ipv4Route.h>
#include <inet/networklayer/ipv4/IIpv4RoutingTable.h>
#include <inet/networklayer/common/L3Tools.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>

#include <inet/linklayer/common/InterfaceTag_m.h>

#include "stack/ip2nic/IP2Nic.h"
#include "stack/mac/LteMacBase.h"
#include "common/binder/Binder.h"
#include "common/cellInfo/CellInfo.h"

namespace simu5g {

using namespace std;
using namespace inet;
using namespace omnetpp;

Define_Module(IP2Nic);

void IP2Nic::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        stackGateOut_ = gate("stackNic$o");
        ipGateOut_ = gate("upperLayerOut");

        setNodeType(par("nodeType").stdstringValue());

        hoManager_.reference(this, "handoverManagerModule", false);

        ueHold_ = false;

        binder_.reference(this, "binderModule", true);

        NetworkInterface *nic = getContainingNicModule(this);
        dualConnectivityEnabled_ = nic->par("dualConnectivityEnabled").boolValue();
        if (dualConnectivityEnabled_)
            sbTable_ = new SplitBearersTable();

        if (nodeType_ == ENODEB || nodeType_ == GNODEB) {
            // TODO: not so elegant
            cModule *bs = getContainingNode(this);
            MacNodeId masterId = MacNodeId(bs->par("masterId").intValue());
            MacNodeId cellId = binder_->registerNode(bs, nodeType_, masterId);
            nodeId_ = cellId;
            nrNodeId_ = NODEID_NONE;
        }
    }
    else if (stage == INITSTAGE_PHYSICAL_ENVIRONMENT) {
        if (nodeType_ == ENODEB || nodeType_ == GNODEB) {
            registerInterface();
        }
        else if (nodeType_ == UE) {
            cModule *ue = getContainingNode(this);

            masterId_ = MacNodeId(ue->par("masterId").intValue());
            nodeId_ = binder_->registerNode(ue, nodeType_, masterId_);

            if (ue->hasPar("nrMasterId") && ue->par("nrMasterId").intValue() != 0) { // register also the NR MacNodeId
                nrMasterId_ = MacNodeId(ue->par("nrMasterId").intValue());
                nrNodeId_ = binder_->registerNode(ue, nodeType_, nrMasterId_, true);
            }
            else
                nrNodeId_ = NODEID_NONE;

            registerInterface();
        }
        else {
            throw cRuntimeError("Unhandled node type: %i", nodeType_);
        }
    }
    else if (stage == inet::INITSTAGE_STATIC_ROUTING) {
        if (nodeType_ == UE) {
            // TODO: shift to routing stage
            // if the UE has been created dynamically, we need to manually add a default route having our cellular interface as output interface
            // otherwise we are not able to reach devices outside the cellular network
            if (NOW > 0) {
                /**
                 * TODO: might need a bit more care, if the interface has changed, the query might, too
                 */
                IIpv4RoutingTable *irt = getModuleFromPar<IIpv4RoutingTable>(
                        par("routingTableModule"), this);
                Ipv4Route *defaultRoute = new Ipv4Route();
                defaultRoute->setDestination(
                        Ipv4Address(inet::Ipv4Address::UNSPECIFIED_ADDRESS));
                defaultRoute->setNetmask(
                        Ipv4Address(inet::Ipv4Address::UNSPECIFIED_ADDRESS));

                defaultRoute->setInterface(networkIf);

                irt->addRoute(defaultRoute);

                // workaround for nodes using the HostAutoConfigurator:
                // Since the HostAutoConfigurator calls setBroadcast(true) for all
                // interfaces in setupNetworking called in INITSTAGE_NETWORK_CONFIGURATION
                // we must reset it to false since the cellular NIC does not support broadcasts
                // at the moment
                networkIf->setBroadcast(false);
            }
        }
    }
    else if (stage == inet::INITSTAGE_TRANSPORT_LAYER) {
        registerMulticastGroups();
    }
}

void IP2Nic::handleMessage(cMessage *msg)
{
    if (nodeType_ == ENODEB || nodeType_ == GNODEB) {
        // message from IP Layer: send to stack
        if (msg->getArrivalGate()->isName("upperLayerIn")) {
            auto ipDatagram = check_and_cast<Packet *>(msg);
            fromIpBs(ipDatagram);
        }
        // message from stack: send to peer
        else if (msg->getArrivalGate()->isName("stackNic$i")) {
            auto pkt = check_and_cast<Packet *>(msg);
            pkt->removeTagIfPresent<SocketInd>();
            removeAllSimu5GTags(pkt);

            toIpBs(pkt);
        }
        else {
            // error: drop message
            EV << "IP2Nic::handleMessage - (E/GNODEB): Wrong gate " << msg->getArrivalGate()->getName() << endl;
            delete msg;
        }
    }
    else if (nodeType_ == UE) {
        // message from transport: send to stack
        if (msg->getArrivalGate()->isName("upperLayerIn")) {

            auto ipDatagram = check_and_cast<Packet *>(msg);
            EV << "LteIp: message from transport: send to stack" << endl;
            fromIpUe(ipDatagram);
        }
        else if (msg->getArrivalGate()->isName("stackNic$i")) {
            // message from stack: send to transport
            EV << "LteIp: message from stack: send to transport" << endl;
            auto pkt = check_and_cast<Packet *>(msg);
            pkt->removeTagIfPresent<SocketInd>();
            removeAllSimu5GTags(pkt);
            toIpUe(pkt);
        }
        else {
            // error: drop message
            EV << "IP2Nic (UE): Wrong gate " << msg->getArrivalGate()->getName() << endl;
            delete msg;
        }
    }
}

void IP2Nic::setNodeType(std::string s)
{
    nodeType_ = aToNodeType(s);
    EV << "Node type: " << s << " -> " << nodeType_ << endl;
}

void IP2Nic::fromIpUe(Packet *datagram)
{
    EV << "IP2Nic::fromIpUe - message from IP layer: send to stack: " << datagram->str() << std::endl;
    // Remove control info from IP datagram
    datagram->removeTagIfPresent<SocketInd>();
    removeAllSimu5GTags(datagram);

    // Remove InterfaceReq Tag (we already are on an interface now)
    datagram->removeTagIfPresent<InterfaceReq>();

    if (ueHold_) {
        // hold packets until handover is complete
        ueHoldFromIp_.push_back(datagram);
    }
    else {
        if (masterId_ == NODEID_NONE && nrMasterId_ == NODEID_NONE) { // UE is detached
            EV << "IP2Nic::fromIpUe - UE is not attached to any serving node. Delete packet." << endl;
            delete datagram;
        }
        else
            toStackUe(datagram);
    }
}

void IP2Nic::toStackUe(Packet *pkt)
{
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    auto srcAddr = ipHeader->getSrcAddress();
    auto destAddr = ipHeader->getDestAddress();
    short int tos = ipHeader->getTypeOfService();
    int headerSize = ipHeader->getHeaderLength().get();
    int transportProtocol = ipHeader->getProtocolId();

    // TODO: Add support for IPv6 (=> see L3Tools.cc of INET)

    // inspect packet depending on the transport protocol type
    // TODO: needs refactoring (redundant code, see toStackBs())
    if (transportProtocol == IP_PROT_TCP) {
        auto tcpHeader = pkt->peekDataAt<tcp::TcpHeader>(ipHeader->getChunkLength());
        headerSize += B(tcpHeader->getHeaderLength()).get();
    }
    else if (transportProtocol == IP_PROT_UDP) {
        headerSize += inet::UDP_HEADER_LENGTH.get();
    }
    else {
        EV_ERROR << "Unknown transport protocol id." << endl;
    }

    pkt->addTagIfAbsent<FlowControlInfo>()->setSrcAddr(srcAddr.getInt());
    pkt->addTagIfAbsent<FlowControlInfo>()->setDstAddr(destAddr.getInt());
    pkt->addTagIfAbsent<FlowControlInfo>()->setHeaderSize(headerSize);
    pkt->addTagIfAbsent<FlowControlInfo>()->setTypeOfService(tos);
    printControlInfo(pkt);

    // mark packet for using NR
    if (!markPacket(pkt->getTagForUpdate<FlowControlInfo>())) {
        EV << "IP2Nic::toStackUe - UE is not attached to any serving node. Delete packet." << endl;
        delete pkt;
    }

    //** Send datagram to LTE stack or LteIp peer **
    send(pkt, stackGateOut_);
}

void IP2Nic::prepareForIpv4(Packet *datagram, const Protocol *protocol) {
    // add DispatchProtocolRequest so that the packet is handled by the specified protocol
    datagram->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(protocol);
    datagram->addTagIfAbsent<PacketProtocolTag>()->setProtocol(protocol);
    // add Interface-Indication to indicate which interface this packet was received from
    datagram->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkIf->getInterfaceId());
}

void IP2Nic::toIpUe(Packet *pkt)
{
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    auto networkProtocolInd = pkt->addTagIfAbsent<NetworkProtocolInd>();
    networkProtocolInd->setProtocol(&Protocol::ipv4);
    networkProtocolInd->setNetworkProtocolHeader(ipHeader);
    prepareForIpv4(pkt);
    EV << "IP2Nic::toIpUe - message from stack: send to IP layer" << endl;
    send(pkt, ipGateOut_);
}

void IP2Nic::fromIpBs(Packet *pkt)
{
    EV << "IP2Nic::fromIpBs - message from IP layer: send to stack" << endl;
    // Remove control info from IP datagram
    pkt->removeTagIfPresent<SocketInd>();
    removeAllSimu5GTags(pkt);

    // Remove InterfaceReq Tag (we already are on an interface now)
    pkt->removeTagIfPresent<InterfaceReq>();

    // TODO: Add support for IPv6
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    const Ipv4Address& destAddr = ipHeader->getDestAddress();

    // handle "forwarding" of packets during handover
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    if (hoForwarding_.find(destId) != hoForwarding_.end()) {
        // data packet must be forwarded (via X2) to another eNB
        MacNodeId targetEnb = hoForwarding_.at(destId);
        sendTunneledPacketOnHandover(pkt, targetEnb);
        return;
    }

    // handle incoming packets destined to UEs that are completing handover
    if (hoHolding_.find(destId) != hoHolding_.end()) {
        // hold packets until handover is complete
        if (hoFromIp_.find(destId) == hoFromIp_.end()) {
            IpDatagramQueue queue;
            hoFromIp_[destId] = queue;
        }

        hoFromIp_[destId].push_back(pkt);
        return;
    }

    toStackBs(pkt);
}

void IP2Nic::toIpBs(Packet *pkt)
{
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    auto networkProtocolInd = pkt->addTagIfAbsent<NetworkProtocolInd>();
    networkProtocolInd->setProtocol(&Protocol::ipv4);
    networkProtocolInd->setNetworkProtocolHeader(ipHeader);
    prepareForIpv4(pkt, &LteProtocol::ipv4uu);
    EV << "IP2Nic::toIpBs - message from stack: send to IP layer" << endl;
    send(pkt, ipGateOut_);
}

void IP2Nic::toStackBs(Packet *pkt)
{
    EV << "IP2Nic::toStackBs - packet is forwarded to stack" << endl;
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    int transportProtocol = ipHeader->getProtocolId();
    auto srcAddr = ipHeader->getSrcAddress();
    auto destAddr = ipHeader->getDestAddress();
    short int tos = ipHeader->getTypeOfService();
    int headerSize = ipHeader->getHeaderLength().get();

    switch (transportProtocol) {
        case IP_PROT_TCP: {
            auto tcpHeader = pkt->peekDataAt<tcp::TcpHeader>(ipHeader->getChunkLength());
            headerSize += B(tcpHeader->getHeaderLength()).get();
            break;
        }
        case IP_PROT_UDP: {
            headerSize += inet::UDP_HEADER_LENGTH.get();
            break;
        }
        default: {
            EV_ERROR << "Unknown transport protocol id." << endl;
        }
    }

    // prepare flow info for NIC
    pkt->addTagIfAbsent<FlowControlInfo>()->setSrcAddr(srcAddr.getInt());
    pkt->addTagIfAbsent<FlowControlInfo>()->setDstAddr(destAddr.getInt());
    pkt->addTagIfAbsent<FlowControlInfo>()->setTypeOfService(tos);
    pkt->addTagIfAbsent<FlowControlInfo>()->setHeaderSize(headerSize);

    // mark packet for using NR
    if (!markPacket(pkt->getTagForUpdate<FlowControlInfo>())) {
        EV << "IP2Nic::toStackBs - UE is not attached to any serving node. Delete packet." << endl;
        delete pkt;
    }
    else {
        printControlInfo(pkt);
        send(pkt, stackGateOut_);
    }
}

void IP2Nic::printControlInfo(Packet *pkt)
{
    EV << "Src IP : " << Ipv4Address(pkt->getTag<FlowControlInfo>()->getSrcAddr()) << endl;
    EV << "Dst IP : " << Ipv4Address(pkt->getTag<FlowControlInfo>()->getDstAddr()) << endl;
    EV << "ToS : " << pkt->getTag<FlowControlInfo>()->getTypeOfService() << endl;
    EV << "Seq Num  : " << pkt->getTag<FlowControlInfo>()->getSequenceNumber() << endl;
    EV << "Header Size : " << pkt->getTag<FlowControlInfo>()->getHeaderSize() << endl;
}

void IP2Nic::registerInterface()
{
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (!ift)
        return;

    networkIf = getContainingNicModule(this);
    networkIf->setInterfaceName(par("interfaceName").stdstringValue().c_str());
    // TODO: configure MTE size from NED
    networkIf->setMtu(1500);
    // Disable broadcast (not supported in cellular NIC), enable multicast
    networkIf->setBroadcast(false);
    networkIf->setMulticast(true);
    networkIf->setLoopback(false);

    // generate a link-layer address to be used as interface token for IPv6
    InterfaceToken token(0, getSimulation()->getUniqueNumber(), 64);
    networkIf->setInterfaceToken(token);

    // capabilities
    networkIf->setMulticast(true);
    networkIf->setPointToPoint(true);
}

void IP2Nic::registerMulticastGroups()
{
    // get all the multicast addresses where the node is enrolled
    NetworkInterface *iface;
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (!ift)
        return;
    iface = ift->findInterfaceByName(par("interfaceName").stdstringValue().c_str());
    unsigned int numOfAddresses = iface->getProtocolData<Ipv4InterfaceData>()->getNumOfJoinedMulticastGroups();

    for (unsigned int i = 0; i < numOfAddresses; ++i) {
        Ipv4Address addr = iface->getProtocolData<Ipv4InterfaceData>()->getJoinedMulticastGroup(i);
        // get the group id and add it to the binder
        uint32_t address = addr.getInt();
        uint32_t mask = ~((uint32_t)255 << 24);      // 00000000 11111111 11111111 11111111
        uint32_t groupId = address & mask;
        binder_->registerMulticastGroup(nodeId_, groupId);
        // register also the NR stack, if any
        if (nrNodeId_ != NODEID_NONE)
            binder_->registerMulticastGroup(nrNodeId_, groupId);
    }
}

bool IP2Nic::markPacket(inet::Ptr<FlowControlInfo> ci)
{
    // In the current version, the IP2Nic module of the master eNB (the UE) selects which path
    // to follow based on the Type of Service (TOS) field:
    // - use master eNB if tos < 10
    // - use secondary gNB if 10 <= tos < 20
    // - use split bearer if tos >= 20
    //
    // To change the policy, change the implementation of the IP2Nic::markPacket() function
    //
    // TODO use a better policy
    // TODO make it configurable from INI or XML?

    if (nodeType_ == ENODEB || nodeType_ == GNODEB) {
        MacNodeId ueId = binder_->getMacNodeId((Ipv4Address)ci->getDstAddr());
        MacNodeId nrUeId = binder_->getNrMacNodeId((Ipv4Address)ci->getDstAddr());
        bool ueLteStack = (binder_->getNextHop(ueId) != NODEID_NONE);
        bool ueNrStack = (binder_->getNextHop(nrUeId) != NODEID_NONE);

        if (dualConnectivityEnabled_ && ueLteStack && ueNrStack && ci->getTypeOfService() >= 20) { // use split bearer TODO fix threshold
            // even packets go through the LTE eNodeB
            // odd packets go through the gNodeB

            int sentPackets;
            if ((sentPackets = sbTable_->find_entry(ci->getSrcAddr(), ci->getDstAddr(), ci->getTypeOfService())) < 0)
                sentPackets = sbTable_->create_entry(ci->getSrcAddr(), ci->getDstAddr(), ci->getTypeOfService());

            if (sentPackets % 2 == 0)
                ci->setUseNR(false);
            else
                ci->setUseNR(true);
        }
        else {
            if (ueLteStack && ueNrStack) {
                if (ci->getTypeOfService() >= 10)                                                     // use secondary cell group bearer TODO fix threshold
                    ci->setUseNR(true);
                else                                                  // use master cell group bearer
                    ci->setUseNR(false);
            }
            else if (ueLteStack)
                ci->setUseNR(false);
            else if (ueNrStack)
                ci->setUseNR(true);
            else
                return false;
        }
    }

    if (nodeType_ == UE) {
        bool ueLteStack = (binder_->getNextHop(nodeId_) != NODEID_NONE);
        bool ueNrStack = (binder_->getNextHop(nrNodeId_) != NODEID_NONE);
        if (dualConnectivityEnabled_ && ueLteStack && ueNrStack && ci->getTypeOfService() >= 20) { // use split bearer TODO fix threshold
            int sentPackets;
            if ((sentPackets = sbTable_->find_entry(ci->getSrcAddr(), ci->getDstAddr(), ci->getTypeOfService())) < 0)
                sentPackets = sbTable_->create_entry(ci->getSrcAddr(), ci->getDstAddr(), ci->getTypeOfService());

            if (sentPackets % 2 == 0)
                ci->setUseNR(false);
            else
                ci->setUseNR(true);
        }
        else {
            if (masterId_ == NODEID_NONE && nrMasterId_ != NODEID_NONE)
                ci->setUseNR(true);
            else if (masterId_ != NODEID_NONE && nrMasterId_ == NODEID_NONE)
                ci->setUseNR(false);
            else {
                // both != 0
                if (ci->getTypeOfService() >= 10)                                                     // use secondary cell group bearer TODO fix threshold
                    ci->setUseNR(true);
                else                                                       // use master cell group bearer
                    ci->setUseNR(false);
            }
        }
    }
    return true;
}

void IP2Nic::triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " IP2Nic::triggerHandoverSource - start tunneling of packets destined to " << ueId << " towards eNB " << targetEnb << endl;

    hoForwarding_[ueId] = targetEnb;

    if (!hoManager_)
        hoManager_.reference(this, "handoverManagerModule", true);

    if (targetEnb != NODEID_NONE)
        hoManager_->sendHandoverCommand(ueId, targetEnb, true);
}

void IP2Nic::triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb)
{
    EV << NOW << " IP2Nic::triggerHandoverTarget - start holding packets destined to " << ueId << endl;

    // reception of handover command from X2
    hoHolding_.insert(ueId);
}

void IP2Nic::sendTunneledPacketOnHandover(Packet *datagram, MacNodeId targetEnb)
{
    EV << "IP2Nic::sendTunneledPacketOnHandover - destination is handing over to eNB " << targetEnb << ". Forward packet via X2." << endl;
    if (!hoManager_)
        hoManager_.reference(this, "handoverManagerModule", true);
    hoManager_->forwardDataToTargetEnb(datagram, targetEnb);
}

void IP2Nic::receiveTunneledPacketOnHandover(Packet *datagram, MacNodeId sourceEnb)
{
    EV << "IP2Nic::receiveTunneledPacketOnHandover - received packet via X2 from " << sourceEnb << endl;
    const auto& hdr = datagram->peekAtFront<Ipv4Header>();
    const Ipv4Address& destAddr = hdr->getDestAddress();
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    if (hoFromX2_.find(destId) == hoFromX2_.end()) {
        IpDatagramQueue queue;
        hoFromX2_[destId] = queue;
    }

    hoFromX2_[destId].push_back(datagram);
}

void IP2Nic::signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " IP2Nic::signalHandoverCompleteSource - handover of UE " << ueId << " to eNB " << targetEnb << " completed!" << endl;
    hoForwarding_.erase(ueId);
}

void IP2Nic::signalHandoverCompleteTarget(MacNodeId ueId, MacNodeId sourceEnb)
{
    Enter_Method("signalHandoverCompleteTarget");

    // signal the event to the source eNB
    if (!hoManager_)
        hoManager_.reference(this, "handoverManagerModule", true);
    hoManager_->sendHandoverCommand(ueId, sourceEnb, false);

    // send down buffered packets in the following order:
    // 1) packets received from X2
    // 2) packets received from IP

    if (hoFromX2_.find(ueId) != hoFromX2_.end()) {
        IpDatagramQueue& queue = hoFromX2_[ueId];
        while (!queue.empty()) {
            Packet *pkt = queue.front();
            queue.pop_front();

            // send pkt down
            take(pkt);
            pkt->trim();
            toStackBs(pkt);
        }
    }

    if (hoFromIp_.find(ueId) != hoFromIp_.end()) {
        IpDatagramQueue& queue = hoFromIp_[ueId];
        while (!queue.empty()) {
            Packet *pkt = queue.front();
            queue.pop_front();

            // send pkt down
            take(pkt);
            toStackBs(pkt);
        }
    }

    hoHolding_.erase(ueId);
}

void IP2Nic::triggerHandoverUe(MacNodeId newMasterId, bool isNr)
{
    EV << NOW << " IP2Nic::triggerHandoverUe - start holding packets" << endl;

    if (newMasterId != NODEID_NONE) {
        ueHold_ = true;
        if (isNr)
            nrMasterId_ = newMasterId;
        else
            masterId_ = newMasterId;
    }
    else {
        if (isNr)
            nrMasterId_ = NODEID_NONE;
        else
            masterId_ = NODEID_NONE;
    }
}

void IP2Nic::signalHandoverCompleteUe(bool isNr)
{
    Enter_Method("signalHandoverCompleteUe");

    if ((!isNr && masterId_ != NODEID_NONE) || (isNr && nrMasterId_ != NODEID_NONE)) {
        // send held packets
        while (!ueHoldFromIp_.empty()) {
            auto pkt = ueHoldFromIp_.front();
            ueHoldFromIp_.pop_front();

            // send pkt down
            take(pkt);
            toStackUe(pkt);
        }
        ueHold_ = false;
    }
}

IP2Nic::~IP2Nic()
{
    for (auto &[macNodeId, ipDatagramQueue] : hoFromX2_) {
        while (!ipDatagramQueue.empty()) {
            Packet *pkt = ipDatagramQueue.front();
            ipDatagramQueue.pop_front();
            delete pkt;
        }
    }

    for (auto &[macNodeId, ipDatagramQueue] : hoFromIp_) {
        while (!ipDatagramQueue.empty()) {
            Packet *pkt = ipDatagramQueue.front();
            ipDatagramQueue.pop_front();
            delete pkt;
        }
    }

    if (dualConnectivityEnabled_)
        delete sbTable_;
}

void IP2Nic::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH) {
        // do this only at deletion of the module during the simulation
        binder_->unregisterNode(nodeId_);
        binder_->unregisterNode(nrNodeId_);
    }
}

} //namespace

