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

#include "simu5g/stack/ip2nic/Ip2Nic.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/cellInfo/CellInfo.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

using namespace std;
using namespace inet;
using namespace omnetpp;

Define_Module(Ip2Nic);

void Ip2Nic::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        stackGateOut_ = gate("stackNic$o");
        ipGateOut_ = gate("upperLayerOut");

        setNodeType(par("nodeType").stdstringValue());

        hoManager_.reference(this, "handoverManagerModule", false);

        binder_.reference(this, "binderModule", true);

        NetworkInterface *nic = getContainingNicModule(this);
        dualConnectivityEnabled_ = nic->par("dualConnectivityEnabled").boolValue();
        if (dualConnectivityEnabled_)
            sbTable_ = new SplitBearersTable();
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS) {
        if (nodeType_ == NODEB) {
            // TODO: not so elegant
            cModule *bs = getContainingNode(this);
            nodeId_ = MacNodeId(bs->par("macNodeId").intValue());
            bool isNr = bs->par("nodeType").stdstringValue() == "GNODEB";
            binder_->registerNode(nodeId_, bs, nodeType_, isNr);

            // display node ID above node icon
            bs->getDisplayString().setTagArg("t", 0, opp_stringf("nodeId=%d", nodeId_).c_str());
        }
        else if (nodeType_ == UE) {
            cModule *ue = getContainingNode(this);
            servingNodeId_ = MacNodeId(ue->par("servingNodeId").intValue());
            nodeId_ = MacNodeId(ue->par("macNodeId").intValue());
            binder_->registerNode(nodeId_, ue, nodeType_, false);
            ue->getDisplayString().setTagArg("t", 0, opp_stringf("nodeId=%d", nodeId_).c_str());

            if (ue->hasPar("nrServingNodeId") && ue->par("nrServingNodeId").intValue() != 0) { // register also the NR MacNodeId
                nrServingNodeId_ = MacNodeId(ue->par("nrServingNodeId").intValue());
                nrNodeId_ = MacNodeId(ue->par("nrMacNodeId").intValue());
                binder_->registerNode(nrNodeId_, ue, nodeType_, true);
                ue->getDisplayString().setTagArg("t", 0, opp_stringf("nodeId=%d/%d", nodeId_, nrNodeId_).c_str());
            }
        }

        registerInterface();
    }
    else if (stage == INITSTAGE_SIMU5G_NODE_RELATIONSHIPS) {
        if (nodeType_ == NODEB) {
            cModule *bs = getContainingNode(this);
            MacNodeId masterId = MacNodeId(bs->par("masterId").intValue());
            binder_->registerMasterNode(masterId, nodeId_);  // note: even if masterId == NODEID_NONE!
        }
        if (nodeType_ == UE) {
            binder_->registerServingNode(servingNodeId_, nodeId_);
            if (nrNodeId_ != NODEID_NONE)
                binder_->registerServingNode(nrServingNodeId_, nrNodeId_);
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
                IIpv4RoutingTable *irt = getModuleFromPar<IIpv4RoutingTable>(par("routingTableModule"), this);
                Ipv4Route *defaultRoute = new Ipv4Route();
                defaultRoute->setDestination(inet::Ipv4Address::UNSPECIFIED_ADDRESS);
                defaultRoute->setNetmask(inet::Ipv4Address::UNSPECIFIED_ADDRESS);

                defaultRoute->setInterface(networkIf);

                irt->addRoute(defaultRoute);

                // workaround for nodes using the HostAutoConfigurator:
                // Since the HostAutoConfigurator calls setBroadcast(true) for all
                // interfaces in setupNetworking called in INITSTAGE_SIMU5G_NETWORK_CONFIGURATION
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

void Ip2Nic::handleMessage(cMessage *msg)
{
    if (nodeType_ == NODEB) {
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
            EV << "Ip2Nic::handleMessage - (E/GNODEB): Wrong gate " << msg->getArrivalGate()->getName() << endl;
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
            EV << "Ip2Nic (UE): Wrong gate " << msg->getArrivalGate()->getName() << endl;
            delete msg;
        }
    }
}

void Ip2Nic::setNodeType(std::string s)
{
    nodeType_ = aToNodeType(s);
    EV << "Node type: " << s << endl;
}

void Ip2Nic::fromIpUe(Packet *datagram)
{
    EV << "Ip2Nic::fromIpUe - message from IP layer: send to stack: " << datagram->str() << std::endl;
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
        if (servingNodeId_ == NODEID_NONE && nrServingNodeId_ == NODEID_NONE) { // UE is detached
            EV << "Ip2Nic::fromIpUe - UE is not attached to any serving node. Delete packet." << endl;
            delete datagram;
        }
        else
            toStackUe(datagram);
    }
}

void Ip2Nic::toStackUe(Packet *pkt)
{
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    auto srcAddr = ipHeader->getSrcAddress();
    auto destAddr = ipHeader->getDestAddress();
    short int tos = ipHeader->getTypeOfService();

    // TODO: Add support for IPv6 (=> see L3Tools.cc of INET)

    auto ipFlowInd = pkt->addTagIfAbsent<IpFlowInd>();
    ipFlowInd->setSrcAddr(srcAddr);
    ipFlowInd->setDstAddr(destAddr);
    ipFlowInd->setTypeOfService(tos);
    printControlInfo(pkt);

    // mark packet for using NR
    bool useNR;
    if (!markPacket(srcAddr, destAddr, tos, useNR)) {
        EV << "Ip2Nic::toStackUe - UE is not attached to any serving node. Delete packet." << endl;
        delete pkt;
        return;
    }

    // Set useNR on the packet control info
    pkt->addTagIfAbsent<TechnologyReq>()->setUseNR(useNR);

    //** Send datagram to LTE stack or LteIp peer **
    send(pkt, stackGateOut_);
}

void Ip2Nic::prepareForIpv4(Packet *datagram, const Protocol *protocol) {
    // add DispatchProtocolRequest so that the packet is handled by the specified protocol
    datagram->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(protocol);
    datagram->addTagIfAbsent<PacketProtocolTag>()->setProtocol(protocol);
    // add Interface-Indication to indicate which interface this packet was received from
    datagram->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkIf->getInterfaceId());
}

void Ip2Nic::toIpUe(Packet *pkt)
{
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    auto networkProtocolInd = pkt->addTagIfAbsent<NetworkProtocolInd>();
    networkProtocolInd->setProtocol(&Protocol::ipv4);
    networkProtocolInd->setNetworkProtocolHeader(ipHeader);
    prepareForIpv4(pkt);
    EV << "Ip2Nic::toIpUe - message from stack: send to IP layer" << endl;
    send(pkt, ipGateOut_);
}

void Ip2Nic::fromIpBs(Packet *pkt)
{
    EV << "Ip2Nic::fromIpBs - message from IP layer: send to stack" << endl;
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

void Ip2Nic::toIpBs(Packet *pkt)
{
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    auto networkProtocolInd = pkt->addTagIfAbsent<NetworkProtocolInd>();
    networkProtocolInd->setProtocol(&Protocol::ipv4);
    networkProtocolInd->setNetworkProtocolHeader(ipHeader);
    prepareForIpv4(pkt, &LteProtocol::ipv4uu);
    EV << "Ip2Nic::toIpBs - message from stack: send to IP layer" << endl;
    send(pkt, ipGateOut_);
}

void Ip2Nic::toStackBs(Packet *pkt)
{
    EV << "Ip2Nic::toStackBs - packet is forwarded to stack" << endl;
    auto ipHeader = pkt->peekAtFront<Ipv4Header>();
    auto srcAddr = ipHeader->getSrcAddress();
    auto destAddr = ipHeader->getDestAddress();
    short int tos = ipHeader->getTypeOfService();

    // prepare flow info for NIC
    auto ipFlowInd = pkt->addTagIfAbsent<IpFlowInd>();
    ipFlowInd->setSrcAddr(srcAddr);
    ipFlowInd->setDstAddr(destAddr);
    ipFlowInd->setTypeOfService(tos);

    // mark packet for using NR
    bool useNR;
    if (!markPacket(srcAddr, destAddr, tos, useNR)) {
        EV << "Ip2Nic::toStackBs - UE is not attached to any serving node. Delete packet." << endl;
        delete pkt;
    }
    else {
        // Set useNR on the packet control info
        pkt->addTagIfAbsent<TechnologyReq>()->setUseNR(useNR);
        printControlInfo(pkt);
        send(pkt, stackGateOut_);
    }
}

void Ip2Nic::printControlInfo(Packet *pkt)
{
    EV << "Src IP : " << pkt->getTag<IpFlowInd>()->getSrcAddr() << endl;
    EV << "Dst IP : " << pkt->getTag<IpFlowInd>()->getDstAddr() << endl;
    EV << "ToS : " << pkt->getTag<IpFlowInd>()->getTypeOfService() << endl;
}

void Ip2Nic::registerInterface()
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

void Ip2Nic::registerMulticastGroups()
{
    // get all the multicast addresses where the node is enrolled
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    NetworkInterface *iface = ift->findInterfaceByName(par("interfaceName").stdstringValue().c_str());
    unsigned int numOfAddresses = iface->getProtocolData<Ipv4InterfaceData>()->getNumOfJoinedMulticastGroups();

    for (unsigned int i = 0; i < numOfAddresses; ++i) {
        Ipv4Address addr = iface->getProtocolData<Ipv4InterfaceData>()->getJoinedMulticastGroup(i);
        MacNodeId multicastDestId = binder_->getOrAssignDestIdForMulticastAddress(addr);
        // register in the LTE and also the NR stack, if any
        binder_->joinMulticastGroup(nodeId_, multicastDestId);
        if (nrNodeId_ != NODEID_NONE)
            binder_->joinMulticastGroup(nrNodeId_, multicastDestId);
    }
}

bool Ip2Nic::markPacket(inet::Ipv4Address srcAddr, inet::Ipv4Address dstAddr, uint16_t typeOfService, bool& useNR)
{
    // In the current version, the Ip2Nic module of the master eNB (the UE) selects which path
    // to follow based on the Type of Service (TOS) field:
    // - use master eNB if tos < 10
    // - use secondary gNB if 10 <= tos < 20
    // - use split bearer if tos >= 20
    //
    // To change the policy, change the implementation of the Ip2Nic::markPacket() function
    //
    // TODO use a better policy
    // TODO make it configurable from INI or XML?

    if (nodeType_ == NODEB) {
        MacNodeId ueId = binder_->getMacNodeId(dstAddr);
        MacNodeId nrUeId = binder_->getNrMacNodeId(dstAddr);
        bool ueLteStack = (binder_->getNextHop(ueId) != NODEID_NONE);
        bool ueNrStack = (binder_->getNextHop(nrUeId) != NODEID_NONE);

        if (dualConnectivityEnabled_ && ueLteStack && ueNrStack && typeOfService >= 20) { // use split bearer TODO fix threshold
            // even packets go through the LTE eNodeB
            // odd packets go through the gNodeB

            int sentPackets;
            if ((sentPackets = sbTable_->find_entry(srcAddr.getInt(), dstAddr.getInt(), typeOfService)) < 0)
                sentPackets = sbTable_->create_entry(srcAddr.getInt(), dstAddr.getInt(), typeOfService);

            if (sentPackets % 2 == 0)
                useNR = false;
            else
                useNR = true;
        }
        else {
            if (ueLteStack && ueNrStack) {
                if (typeOfService >= 10)                                                     // use secondary cell group bearer TODO fix threshold
                    useNR = true;
                else                                                  // use master cell group bearer
                    useNR = false;
            }
            else if (ueLteStack)
                useNR = false;
            else if (ueNrStack)
                useNR = true;
            else
                return false;
        }
    }

    if (nodeType_ == UE) {
        bool ueLteStack = (binder_->getNextHop(nodeId_) != NODEID_NONE);
        bool ueNrStack = (binder_->getNextHop(nrNodeId_) != NODEID_NONE);
        if (dualConnectivityEnabled_ && ueLteStack && ueNrStack && typeOfService >= 20) { // use split bearer TODO fix threshold
            int sentPackets;
            if ((sentPackets = sbTable_->find_entry(srcAddr.getInt(), dstAddr.getInt(), typeOfService)) < 0)
                sentPackets = sbTable_->create_entry(srcAddr.getInt(), dstAddr.getInt(), typeOfService);

            if (sentPackets % 2 == 0)
                useNR = false;
            else
                useNR = true;
        }
        else {
            if (servingNodeId_ == NODEID_NONE && nrServingNodeId_ != NODEID_NONE)
                useNR = true;
            else if (servingNodeId_ != NODEID_NONE && nrServingNodeId_ == NODEID_NONE)
                useNR = false;
            else {
                // both != 0
                if (typeOfService >= 10)                                                     // use secondary cell group bearer TODO fix threshold
                    useNR = true;
                else                                                       // use master cell group bearer
                    useNR = false;
            }
        }
    }
    return true;
}

void Ip2Nic::triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " Ip2Nic::triggerHandoverSource - start tunneling of packets destined to " << ueId << " towards eNB " << targetEnb << endl;

    hoForwarding_[ueId] = targetEnb;

    if (!hoManager_)
        hoManager_.reference(this, "handoverManagerModule", true);

    if (targetEnb != NODEID_NONE)
        hoManager_->sendHandoverCommand(ueId, targetEnb, true);
}

void Ip2Nic::triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb)
{
    EV << NOW << " Ip2Nic::triggerHandoverTarget - start holding packets destined to " << ueId << endl;

    // reception of handover command from X2
    hoHolding_.insert(ueId);
}

void Ip2Nic::sendTunneledPacketOnHandover(Packet *datagram, MacNodeId targetEnb)
{
    EV << "Ip2Nic::sendTunneledPacketOnHandover - destination is handing over to eNB " << targetEnb << ". Forward packet via X2." << endl;
    if (!hoManager_)
        hoManager_.reference(this, "handoverManagerModule", true);
    hoManager_->forwardDataToTargetEnb(datagram, targetEnb);
}

void Ip2Nic::receiveTunneledPacketOnHandover(Packet *datagram, MacNodeId sourceEnb)
{
    EV << "Ip2Nic::receiveTunneledPacketOnHandover - received packet via X2 from " << sourceEnb << endl;
    const auto& hdr = datagram->peekAtFront<Ipv4Header>();
    const Ipv4Address& destAddr = hdr->getDestAddress();
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    if (hoFromX2_.find(destId) == hoFromX2_.end()) {
        IpDatagramQueue queue;
        hoFromX2_[destId] = queue;
    }

    hoFromX2_[destId].push_back(datagram);
}

void Ip2Nic::signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " Ip2Nic::signalHandoverCompleteSource - handover of UE " << ueId << " to eNB " << targetEnb << " completed!" << endl;
    hoForwarding_.erase(ueId);
}

void Ip2Nic::signalHandoverCompleteTarget(MacNodeId ueId, MacNodeId sourceEnb)
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

void Ip2Nic::triggerHandoverUe(MacNodeId newMasterId, bool isNr)
{
    EV << NOW << " Ip2Nic::triggerHandoverUe - start holding packets" << endl;

    if (newMasterId != NODEID_NONE) {
        ueHold_ = true;
        if (isNr)
            nrServingNodeId_ = newMasterId;
        else
            servingNodeId_ = newMasterId;
    }
    else {
        if (isNr)
            nrServingNodeId_ = NODEID_NONE;
        else
            servingNodeId_ = NODEID_NONE;
    }
}

void Ip2Nic::signalHandoverCompleteUe(bool isNr)
{
    Enter_Method("signalHandoverCompleteUe");

    if ((!isNr && servingNodeId_ != NODEID_NONE) || (isNr && nrServingNodeId_ != NODEID_NONE)) {
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

Ip2Nic::~Ip2Nic()
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

void Ip2Nic::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH) {
        // do this only at deletion of the module during the simulation
        binder_->unregisterNode(nodeId_);
        binder_->unregisterNode(nrNodeId_);
    }
}

} //namespace
