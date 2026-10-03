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
#include "simu5g/corenetwork/gtp/GtpUser.h"
#include "simu5g/corenetwork/trafficFlowFilter/TftControlInfo_m.h"
#include "simu5g/common/L3Utils.h"
#include "simu5g/common/QfiTag_m.h"
#include "simu5g/common/SessionTag_m.h"
#include "simu5g/common/UplinkUeTag_m.h"
#include <iostream>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/common/packet/printer/PacketPrinter.h>
#include <inet/common/socket/SocketTag_m.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/linklayer/ethernet/common/Ethernet.h>
#include <inet/linklayer/ethernet/common/EthernetMacHeader_m.h>
#include <inet/networklayer/common/L3Tools.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/networklayer/ipv6/Ipv6Header.h>
#include <inet/networklayer/ipv6/Ipv6InterfaceData.h>
#include <inet/transportlayer/common/L4Tools.h>
#include <inet/transportlayer/udp/Udp.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>

namespace simu5g {

Define_Module(GtpUser);

using namespace omnetpp;
using namespace inet;

void GtpUser::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        networkNode_ = getContainingNode(this);

        ownerType_ = selectOwnerType(networkNode_->par("nodeType"));

        if (isBaseStation(ownerType_))
            myMacNodeID = MacNodeId(networkNode_->par("macNodeId").intValue());
        else
            myMacNodeID = NODEID_NONE;

        // the core network gateway: that of a base station (unless it is a secondary
        // node, not connected to the core network), or of a MEC host's UPF
        bool connectedBS = isBaseStation(ownerType_) && networkNode_->gate("ppp$o")->isConnected();
        if (connectedBS || ownerType_ == UPF_MEC) {
            gateway_ = par("gateway").stdstringValue();
            if (gateway_.empty())
                throw cRuntimeError("The required 'gateway' parameter is empty.");
        }

        binder_.reference(this, "binderModule", true);
        return;
    }

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = par("localPort");

    // transport layer access
    socket_.setOutputGate(gate("socketOut"));
    socket_.bind(localPort_);

    tunnelPeerPort_ = par("tunnelPeerPort");

    // find the address of the core network gateway
    if (!gateway_.empty())
        gwAddress_ = L3AddressResolver().resolve((binder_->getNetworkName() + "." + gateway_).c_str());
}

void GtpUser::addTunnel(Teid teid, const SessionRef& session, SessionType type)
{
    Enter_Method_Silent("addTunnel");
    ASSERT(teid != TEID_NONE);
    if (!rxTunnels_.emplace(teid, TunnelSession{session, type}).second)
        throw cRuntimeError("GtpUser::addTunnel - TEID %u is already in use", num(teid));
    EV_INFO << "GtpUser::addTunnel - TEID " << teid << " belongs to " << session << " (" << sessionTypeToA(type) << ")" << endl;
}

void GtpUser::setUplinkTunnels(const SessionRef& session, SessionType type, const UplinkTunnels& tunnels)
{
    Enter_Method_Silent("setUplinkTunnels");
    ASSERT(isBaseStation(ownerType_));
    for (MacNodeId nodeId : {session.lteNodeId, session.nrNodeId})
        if (nodeId != NODEID_NONE)
            ulTunnels_[nodeId] = UplinkSession{TunnelSession{session, type}, tunnels};
    EV_INFO << "GtpUser::setUplinkTunnels - " << session << " enters the core network at " << tunnels.anchor << endl;
}

void GtpUser::setDownlinkTunnel(const SessionRef& session, const FTeid& tunnel)
{
    Enter_Method_Silent("setDownlinkTunnel");
    ASSERT(!isBaseStation(ownerType_));
    for (MacNodeId nodeId : {session.lteNodeId, session.nrNodeId}) {
        if (nodeId == NODEID_NONE)
            continue;
        if (tunnel.isSet())
            dlTunnels_[nodeId] = tunnel;
        else
            dlTunnels_.erase(nodeId);
    }
    EV_INFO << "GtpUser::setDownlinkTunnel - the downlink of " << session << " is tunneled to " << tunnel << endl;
}

void GtpUser::setN6Tunnel(const SessionRef& session, const N6Tunnel& tunnel)
{
    Enter_Method_Silent("setN6Tunnel");
    ASSERT(ownerType_ == UPF);
    for (MacNodeId nodeId : {session.lteNodeId, session.nrNodeId})
        if (nodeId != NODEID_NONE)
            n6Tunnels_[nodeId] = tunnel;
    EV_INFO << "GtpUser::setN6Tunnel - the uplink of " << session << " leaves on the N6 tunnel " << tunnel << endl;
}

void GtpUser::removeSession(const SessionRef& session)
{
    Enter_Method_Silent("removeSession");
    for (MacNodeId nodeId : {session.lteNodeId, session.nrNodeId}) {
        ulTunnels_.erase(nodeId);
        dlTunnels_.erase(nodeId);
        n6Tunnels_.erase(nodeId);
    }
}

CoreNodeType GtpUser::selectOwnerType(const char *type)
{
    EV << "GtpUser::selectOwnerType - setting owner type to " << type << endl;
    if (strcmp(type, "ENODEB") == 0)
        return ENB;
    else if (strcmp(type, "GNODEB") == 0)
        return GNB;
    else if (strcmp(type, "PGW") == 0)
        return PGW;
    else if (strcmp(type, "UPF") == 0)
        return UPF;
    else if (strcmp(type, "UPF_MEC") == 0)
        return UPF_MEC;

    throw cRuntimeError("GtpUser::selectOwnerType - unknown owner type [%s]", type);

    // you should not be here
    return ENB;
}

void GtpUser::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("trafficFlowFilterGate")) {
        EV << "GtpUser::handleMessage - message from trafficFlowFilter" << endl;

        // forward the encapsulated IP datagram
        handleFromTrafficFlowFilter(check_and_cast<Packet *>(msg));
    }
    else if (msg->arrivedOn("socketIn")) {
        EV << "GtpUser::handleMessage - message from UDP layer" << endl;
        Packet *packet = check_and_cast<Packet *>(msg);
        PacketPrinter printer; // turns packets into human-readable strings
        printer.printPacket(EV, packet); // print to standard output

        handleFromUdp(packet);
    }
    else if (msg->arrivedOn("ndIn")) {
        EV << "GtpUser::handleMessage - message from the Neighbor Discovery responder" << endl;
        handleFromNdResponder(check_and_cast<Packet *>(msg));
    }
}

void GtpUser::handleFromTrafficFlowFilter(Packet *datagram)
{
    /*
     * when we get here, it means that the packet is entering the core network and it may need to be tunneled to some destination,
     * based on the outcome the TrafficFlowFilter found:
     * 1) TFT_REMOVED_DESTINATION: the destination does not belong to the simulation anymore
     *    --> delete the packet
     * 2) TFT_LOCAL_DELIVERY: we are on a BS and the destination is a UE under the same gNB
     *    --> forward the packet to the local LTE/NR NIC
     * 3) TFT_EXTERNAL_DESTINATION: destination is outside the radio network
     *    --> tunnel the packet towards the CN gateway
     * 4) TFT_MEC_HOST: destination is a MEC host
     *    4a) the MEC host is inside the same core network
     *        --> tunnel the packet towards the MEC host
     *    4b) the MEC host is inside another core network
     *        --> tunnel the packet towards the CN gateway
     * 5) TFT_PDU_SESSION: destination is a UE (only at a UPF/PGW or a MEC host's UPF)
     *    5a) its session is served here
     *        --> tunnel the packet on the session's downlink tunnel
     *    5b) the UE is attached nowhere
     *        --> delete the packet
     *    5c) the UE is inside another network
     *        --> tunnel the packet towards the CN gateway
     */

    auto tftInfo = datagram->removeTag<TftControlInfo>();
    TftOutcome tft = tftInfo->getTft();
    Qfi qfi = tftInfo->getQfi();

    // at a base station: the UE the datagram came from over the air (see Ip2Nic)
    MacNodeId sourceUe = isBaseStation(ownerType_) ? datagram->removeTag<UplinkUeTag>()->getUeNodeId() : NODEID_NONE;

    EV << "GtpUser::handleFromTrafficFlowFilter - Received a tftMessage with tft[" << tft << "] qfi[" << qfi << "]" << endl;

    // the downlink of a UE's session goes on the session's tunnel, which the path
    // switch keeps pointing at the base station the downlink enters the RAN at
    const FTeid *dlTunnel = nullptr;
    // an Ethernet session's frame, from the UPF's Ethernet session bridge
    bool ethernetFrame = tft == TFT_PDU_SESSION && datagram->getTag<PacketProtocolTag>()->getProtocol() == &Protocol::ethernetMac;
    if (tft == TFT_PDU_SESSION) {
        MacNodeId ueNodeId = tftInfo->getUeNodeId();
        dlTunnel = findDownlinkTunnel(ueNodeId);
        if (dlTunnel == nullptr) {
            // a UE attached nowhere is dropped; the UE of another core network is its
            // gateway's to reach (a frame of an Ethernet session bridgeed here has no other
            // way to its UE)
            bool attached = binder_->getServingNodeOrSelf(ueNodeId) != NODEID_NONE;
            tft = attached && !ethernetFrame ? TFT_EXTERNAL_DESTINATION : TFT_REMOVED_DESTINATION;
        }
    }

    if (tft == TFT_REMOVED_DESTINATION) {
        // the destination has been removed from the simulation. Delete datagram
        EV << "GtpUser::handleFromTrafficFlowFilter - Destination has been removed from the simulation. Deleting packet." << endl;
        delete datagram;
        return;
    }

    // on a base station, a UE-to-UE shortcut: forward the packet locally
    if (tft == TFT_LOCAL_DELIVERY) {
        // local delivery: keep the classified QFI with the packet, the same way
        // handleFromUdp() restores it for tunneled traffic -- SDAP relies on
        // QfiReq being present on the gNB DL path
        datagram->addTagIfAbsent<QfiReq>()->setQfi(qfi);
        // no tunnel names the destination UE on this shortcut between two UEs of this
        // base station: the traffic flow filter chose it by the destination address,
        // which names one of the UEs whose sessions enter the core network here
        const L3Address& destAddr = datagram->getTag<IpHeaderFieldsTag>()->getDestAddress();
        MacNodeId destUe = binder_->getMacNodeId(destAddr);
        if (destUe == NODEID_NONE)
            destUe = binder_->getNrMacNodeId(destAddr);
        attachSessionTag(datagram, getServedSession(destUe));
        send(datagram, "pppGate");  // to the cellular NIC
    }
    else {
        // the packet is ready to be tunneled via GTP to another node in the core network
        L3Address tunnelPeerAddress;
        Teid teid = TEID_NONE;
        // the QFI travels in a PDU Session Container on the tunnels of 5GC sessions (N3)
        PduSessionContainerType container = PDU_SESSION_CONTAINER_NONE;
        if (tft == TFT_EXTERNAL_DESTINATION) { // send to the gateway
            if (gwAddress_.isUnspecified())
                throw cRuntimeError("Packet is destined by TFT to external destination (Internet), but gateway address is not configured");
            EV << "GtpUser::handleFromTrafficFlowFilter - tunneling to " << gwAddress_.str() << endl;
            tunnelPeerAddress = gwAddress_;
            // a base station sends into the uplink tunnel of the UE's session, whose
            // anchor is the gateway; a MEC host's UPF relays traffic of no session
            if (isBaseStation(ownerType_)) {
                const UplinkTunnels& tunnels = getUplinkTunnels(sourceUe);
                ASSERT(tunnels.anchor.address == gwAddress_);
                teid = tunnels.anchor.teid;
                if (tunnels.toUpf)
                    container = UL_PDU_SESSION_INFORMATION;
            }
        }
        else if (tft == TFT_MEC_HOST) { // send to a MEC host
            // check if the destination MEC host is within the same core network
            L3Address destAddr = datagram->getTag<IpHeaderFieldsTag>()->getDestAddress();

            // retrieve the address of the UPF included within the MEC host
            EV << "GtpUser::handleFromTrafficFlowFilter - tunneling to " << destAddr.str() << endl;
            tunnelPeerAddress = binder_->getUpfFromMecHost(destAddr);
            // a base station sends into the UE's session's uplink tunnel to the MEC
            // host; a UPF relays traffic of no session
            if (isBaseStation(ownerType_)) {
                const UplinkTunnels& tunnels = getUplinkTunnels(sourceUe);
                auto it = tunnels.mecHosts.find(tunnelPeerAddress);
                if (it == tunnels.mecHosts.end())
                    throw cRuntimeError("GtpUser: the session of UE %d has no uplink tunnel to the MEC host UPF %s",
                            num(sourceUe), tunnelPeerAddress.str().c_str());
                teid = it->second;
                container = UL_PDU_SESSION_INFORMATION;
            }
        }
        else { // on the downlink tunnel of the destination UE's session
            ASSERT(tft == TFT_PDU_SESSION && dlTunnel != nullptr);
            // an Ethernet frame enters the session without its FCS (TS 23.501 5.6.10.2)
            if (ethernetFrame)
                datagram->removeAtBack<EthernetFcs>(ETHER_FCS_BYTES);
            EV << "GtpUser::handleFromTrafficFlowFilter - tunneling to " << *dlTunnel << endl;
            tunnelPeerAddress = dlTunnel->address;
            teid = dlTunnel->teid;
            if (ownerType_ != PGW)
                container = DL_PDU_SESSION_INFORMATION;
        }

        // create a new GtpUserMessage and encapsulate the datagram within the GtpUserMessage
        auto header = makeGtpUserHeader(teid, qfi, container, datagram->getDataLength());
        auto gtpPacket = new Packet(datagram->getName());
        gtpPacket->insertAtFront(header);
        auto data = datagram->peekData();
        gtpPacket->insertAtBack(data);

        delete datagram;

        socket_.sendTo(gtpPacket, tunnelPeerAddress, tunnelPeerPort_);
    }
}

void GtpUser::handleFromUdp(Packet *pkt)
{
    /*
     * when we get here, it means that the packet reached the end of the tunnel and it needs to be decapsulated.
     * The following cases can occur:
     * 1) the packet has been received by a BS (this GtpUser module is inside a BS)
     *    --> the destination is for sure a UE served by this BS, hence we decapsulate the packet and deliver it locally
     * 2) the packet has been received by a MEC host (this GtpUser module is inside a MEC host's UPF)
     *    --> the destination is for sure the MEC host, hence we decapsulate the packet and deliver it locally
     * 3) the packet has been received by a "border" PGW/UPF (this GtpUser is inside a PGW/UPF)
     *    3a) the destination of the packet is NOT a UE
     *        --> the destination is for sure outside this network (e.g. a remote server or a node within another radio network),
     *            hence we decapsulate the packet and deliver it to the outbound interface
     *    3b) the destination of the packet is a UE
     *        3b1) the serving BS of the UE does NOT belong to the same radio network as the PGW/UPF
     *             --> we decapsulate the packet and deliver it to the outbound interface
     *        3b2) the serving BS of the UE belongs to the same radio network as the PGW/UPF
     *             --> we decapsulate the packet, re-encapsulate it and send it to the correct BS
     */

    EV << "GtpUser::handleFromUdp - Decapsulating and forwarding to the correct destination" << endl;

    if (const auto& header = pkt->peekAtFront<GtpUserMsg>(); header->getMessageType() == GTPU_END_MARKER) {
        handleEndMarker(header->getTeid());
        delete pkt;
        return;
    }

    // re-create the original datagram of the session and send it to the local network
    auto originalPacket = new Packet(pkt->getName());
    auto gtpUserMsg = pkt->popAtFront<GtpUserMsg>();
    originalPacket->insertAtBack(pkt->peekData());

    // The tunnel names the session the T-PDU belongs to, and so what it carries (a
    // base station knows every tunnel ending here); a relay between a MEC host's UPF
    // and its anchor carries IP traffic of no session
    const TunnelSession *session = (isBaseStation(ownerType_) || gtpUserMsg->getTeid() != TEID_NONE) ? &findTunnel(gtpUserMsg->getTeid()) : nullptr;
    SessionType sessionType = session != nullptr ? session->type : IP_V4;
    originalPacket->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&sessionPayloadProtocol(sessionType, originalPacket));

    // Restore QFI from GTP-U header so SDAP can use it for QFI-to-DRB mapping.
    // Always set the tag, even for QFI 0 (unmarked/default-flow traffic): SDAP
    // relies on QfiReq being present on the gNB DL path, and maps QFI 0 to the
    // default DRB.
    originalPacket->addTagIfAbsent<QfiReq>()->setQfi(gtpUserMsg->getQfi());
    // remove any pending socket indications
    auto sockInd = pkt->removeTagIfPresent<SocketInd>();

    delete pkt;

    if (isBaseStation(ownerType_)) {
        // the tunnel names the session, and so the UE, the datagram is for
        EV << "GtpUser::handleFromUdp - Datagram of " << session->ref << ", local delivery to the cellular NIC" << endl;
        attachSessionTag(originalPacket, *session);
        send(originalPacket, "pppGate");
    }
    else if (ownerType_ == UPF_MEC) {
        // a tunnel from a base station names the session the datagram belongs to; a
        // relay from the UPF carries traffic of no session
        if (session != nullptr)
            EV << "GtpUser::handleFromUdp - Datagram of " << session->ref << endl;

        // we are on the MEC, local delivery
        EV << "GtpUser::handleFromUdp - Datagram local delivery to the MEC host" << endl;
        send(originalPacket, "pppGate");
    }
    else if (ownerType_ == PGW || ownerType_ == UPF) {
        // a tunnel from a base station names the session the datagram belongs to; a
        // relay from a MEC host's UPF carries traffic of no session
        if (session != nullptr)
            EV << "GtpUser::handleFromUdp - Datagram of " << session->ref << endl;

        // the uplink of an Unstructured session goes to its server over the session's
        // N6 tunnel
        if (session != nullptr && session->type == UNSTRUCTURED) {
            tunnelUplinkOverN6(originalPacket, session->ref);
            return;
        }

        // the uplink of an Ethernet session goes into the UPF's bridge, with its FCS
        // rebuilt, tagged with its session
        if (session != nullptr && session->type == ETHERNET) {
            if (!gate("ethernetBridgeOut")->isConnected() || !gate("ethernetBridgeOut")->isPathOK())
                throw cRuntimeError("GtpUser: an uplink frame of the Ethernet %s arrived at %s, which has no Ethernet session bridge (see its hasEthernetBridge parameter)",
                        (std::ostringstream() << session->ref).str().c_str(), getContainingNode(this)->getFullPath().c_str());
            insertDeclaredEthernetFcs(originalPacket);
            originalPacket->removeTagIfPresent<QfiReq>();
            attachSessionTag(originalPacket, *session);
            send(originalPacket, "ethernetBridgeOut");
            return;
        }

        // where the datagram goes next is the destination's matter: the data network, or
        // the session of another UE
        L3Address destAddr = peekIpHeader(originalPacket)->getDestinationAddress();

        // The IP link of a UE's IPv6 session ends here: link-local-scope traffic (Neighbor
        // Discovery) is answered at this node and must not leak onto the data network
        if (destAddr.getType() == L3Address::IPv6 && isLinkLocalScope(destAddr.toIpv6())) {
            if (!gate("ndOut")->isConnected() || !gate("ndOut")->isPathOK())
                throw cRuntimeError("GtpUser: link-local IPv6 traffic (destination %s) from a UE arrived, but this node has no Neighbor Discovery responder (see the hasNdResponder parameter)", destAddr.str().c_str());
            send(originalPacket, "ndOut");
            return;
        }

        MacNodeId destId = binder_->getMacNodeId(destAddr);
        if (destId != NODEID_NONE) { // final destination is a UE
            // a UE whose session is served here: into the downlink tunnel of that
            // session, preserving the QFI of the incoming GTP-U
            if (const FTeid *tunnel = findDownlinkTunnel(destId)) {
                tunnelDownlink(originalPacket, *tunnel, gtpUserMsg->getQfi());
                return;
            }
            // a UE attached nowhere has no tunnel to reach it by, as for downlink traffic
            // entering the core network; the UE of another core network is reached
            // through the data network
            if (binder_->getServingNodeOrSelf(destId) == NODEID_NONE) {
                EV << "GtpUser::handleFromUdp - Destination " << destAddr << " is a UE attached nowhere, deleting datagram" << endl;
                delete originalPacket;
                return;
            }
        }

        // destination is outside the radio network
        EV << "GtpUser::handleFromUdp - Sending datagram outside the radio network, destination[" << destAddr.str() << "]" << endl;
        send(originalPacket, "pppGate");
    }
}

void GtpUser::handleFromNdResponder(Packet *datagram)
{
    L3Address destAddr = peekIpHeader(datagram)->getDestinationAddress();
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    if (destId == NODEID_NONE)
        throw cRuntimeError("GtpUser: the Neighbor Discovery responder answered %s, which is no UE's address", destAddr.str().c_str());

    const FTeid *tunnel = findDownlinkTunnel(destId);
    if (tunnel == nullptr) {
        if (binder_->getServingNodeOrSelf(destId) != NODEID_NONE)
            throw cRuntimeError("GtpUser: the Neighbor Discovery responder answered %s, a UE whose session is not served here", destAddr.str().c_str());
        EV_WARN << "GtpUser::handleFromNdResponder - UE " << destId << " is attached to no base station, reply to " << destAddr << " discarded" << endl;
        delete datagram;
        return;
    }
    tunnelDownlink(datagram, *tunnel, Qfi(0));  // the default QoS flow
}

void GtpUser::tunnelDownlink(Packet *datagram, const FTeid& tunnel, Qfi qfi)
{
    EV << "GtpUser::tunnelDownlink - tunneling to " << tunnel << endl;

    // send the message to the BS through GTP tunneling
    // * create a new GtpUserMessage
    // * encapsulate the datagram within the GtpUserMsg
    auto header = makeGtpUserHeader(tunnel.teid, qfi, ownerType_ == PGW ? PDU_SESSION_CONTAINER_NONE : DL_PDU_SESSION_INFORMATION, datagram->getDataLength());
    auto gtpMsg = new Packet(datagram->getName());
    gtpMsg->insertAtFront(header);
    auto data = datagram->peekData();
    gtpMsg->insertAtBack(data);
    delete datagram;

    socket_.sendTo(gtpMsg, tunnel.address, tunnelPeerPort_);
}

void GtpUser::tunnelUplinkOverN6(Packet *payload, const SessionRef& session)
{
    auto it = n6Tunnels_.find(session.lteNodeId);
    if (it == n6Tunnels_.end())
        throw cRuntimeError("GtpUser: the Unstructured session of UE %d has no N6 tunnel at %s", (int)num(session.lteNodeId), getContainingNode(this)->getFullPath().c_str());
    const N6Tunnel& tunnel = it->second;
    EV << "GtpUser::tunnelUplinkOverN6 - uplink of " << session << " over the N6 tunnel " << tunnel << endl;

    // the N6 tunnel's UDP header, from the port the UPF receives the downlink on
    bool ipv6 = tunnel.serverAddress.getType() == L3Address::IPv6;
    const Protocol& ipProtocol = ipv6 ? Protocol::ipv6 : Protocol::ipv4;
    auto udpHeader = makeShared<UdpHeader>();
    udpHeader->setSourcePort(tunnel.localPort);
    udpHeader->setDestinationPort(tunnel.serverPort);
    udpHeader->setTotalLengthField(udpHeader->getChunkLength() + payload->getDataLength());
    udpHeader->setChecksumMode(CHECKSUM_DECLARED_CORRECT);
    Udp::insertChecksum(&ipProtocol, tunnel.sessionAddress, tunnel.serverAddress, udpHeader, payload);
    insertTransportProtocolHeader(payload, Protocol::udp, udpHeader);

    // and its IP header, from the session's address
    if (ipv6) {
        auto ipHeader = makeShared<Ipv6Header>();
        ipHeader->setSrcAddress(tunnel.sessionAddress.toIpv6());
        ipHeader->setDestAddress(tunnel.serverAddress.toIpv6());
        ipHeader->setHopLimit(IPv6_DEFAULT_ADVCURHOPLIMIT);
        ipHeader->setProtocolId(IP_PROT_UDP);
        ipHeader->setPayloadLength(payload->getDataLength());
        insertNetworkProtocolHeader(payload, Protocol::ipv6, ipHeader);
    }
    else {
        auto ipHeader = makeShared<Ipv4Header>();
        ipHeader->setSrcAddress(tunnel.sessionAddress.toIpv4());
        ipHeader->setDestAddress(tunnel.serverAddress.toIpv4());
        ipHeader->setTimeToLive(32);
        ipHeader->setProtocolId(IP_PROT_UDP);
        ipHeader->setIdentification(n6DatagramId_++);
        ipHeader->setHeaderLength(ipHeader->getChunkLength());
        ipHeader->setTotalLengthField(ipHeader->getChunkLength() + payload->getDataLength());
        ipHeader->setChecksumMode(CHECKSUM_DECLARED_CORRECT);
        ipHeader->updateChecksum();
        insertNetworkProtocolHeader(payload, Protocol::ipv4, ipHeader);
    }
    send(payload, "pppGate");
}

const UplinkTunnels& GtpUser::getUplinkTunnels(MacNodeId ueNodeId)
{
    auto it = ulTunnels_.find(ueNodeId);
    if (it == ulTunnels_.end())
        throw cRuntimeError("GtpUser: the session of UE %d has no uplink tunnel from here", num(ueNodeId));
    return it->second.tunnels;
}

void GtpUser::sendEndMarker(const FTeid& tunnel)
{
    Enter_Method("sendEndMarker");
    if (ownerType_ != UPF && ownerType_ != PGW)
        throw cRuntimeError("GtpUser: %s is no anchor UPF/PGW to send an End Marker", getFullPath().c_str());
    EV << "GtpUser::sendEndMarker - the downlink leaves " << tunnel << endl;
    auto header = makeGtpUserHeader(tunnel.teid, QFI_NONE, PDU_SESSION_CONTAINER_NONE, B(0));
    header->setMessageType(GTPU_END_MARKER);
    auto endMarker = new Packet("GtpEndMarker");
    endMarker->insertAtFront(header);
    socket_.sendTo(endMarker, tunnel.address, tunnelPeerPort_);
}

void GtpUser::handleEndMarker(Teid teid)
{
    if (!isBaseStation(ownerType_))
        throw cRuntimeError("GtpUser: an End Marker arrived at %s, which is no base station", getFullPath().c_str());
    // the end of the session's downlink on this tunnel: the handover source relays it
    // to where the UE went (see HandoverPacketHolderEnb)
    const TunnelSession& session = findTunnel(teid);
    EV << "GtpUser::handleEndMarker - End Marker of " << session.ref << endl;
    auto endMarker = new Packet("GtpEndMarker");
    attachSessionTag(endMarker, session);
    endMarker->addTag<GtpEndMarkerInd>();
    send(endMarker, "pppGate");
}

const FTeid *GtpUser::findDownlinkTunnel(MacNodeId ueNodeId)
{
    auto it = dlTunnels_.find(ueNodeId);
    return (it != dlTunnels_.end() && it->second.isSet()) ? &it->second : nullptr;
}

const TunnelSession& GtpUser::getServedSession(MacNodeId ueNodeId)
{
    auto it = ulTunnels_.find(ueNodeId);
    if (it == ulTunnels_.end())
        throw cRuntimeError("GtpUser: UE %d has no session entering the core network here", num(ueNodeId));
    return it->second.session;
}

const TunnelSession& GtpUser::findTunnel(Teid teid)
{
    auto it = rxTunnels_.find(teid);
    if (it == rxTunnels_.end())
        throw cRuntimeError("GtpUser: a G-PDU arrived with TEID %u, which is no tunnel ending here", num(teid));
    return it->second;
}

} //namespace
