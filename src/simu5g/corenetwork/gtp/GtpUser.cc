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
#include "simu5g/corenetwork/bearerConfigurator/BearerConfigurator.h"
#include "simu5g/corenetwork/trafficFlowFilter/TftControlInfo_m.h"
#include "simu5g/common/L3Utils.h"
#include "simu5g/common/QfiTag_m.h"
#include <iostream>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/common/packet/printer/PacketPrinter.h>
#include <inet/common/socket/SocketTag_m.h>
#include <inet/linklayer/common/InterfaceTag_m.h>

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

        // announce this tunnel endpoint to the bearer configurator, which allocates the
        // tunnel endpoint ids of the PDU sessions' tunnels, standing in for the SMF
        bearerConfigurator_.reference(this, "bearerConfiguratorModule", true);
        bearerConfigurator_->registerGtpEndpoint(this, ownerType_, myMacNodeID, gateway_);
        return;
    }

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = par("localPort");

    // get reference to the binder
    binder_.reference(this, "binderModule", true);

    // transport layer access
    socket_.setOutputGate(gate("socketOut"));
    socket_.bind(localPort_);

    tunnelPeerPort_ = par("tunnelPeerPort");

    // find the address of the core network gateway
    if (!gateway_.empty())
        gwAddress_ = L3AddressResolver().resolve((binder_->getNetworkName() + "." + gateway_).c_str());
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
     * based on the trafficFlowId found by the TrafficFlowFilter:
     * 1) tftId == -2, the destination does not belong to the simulation anymore
     *    --> delete the packet
     * 2) tftId == 0, we are on a BS and the destination is a UE under the same gNB
     *    --> forward the packet to the local LTE/NR NIC
     * 3) tftId == -1, destination is outside the radio network
     *    --> tunnel the packet towards the CN gateway
     * 4) tftId == -3, destination is a MEC host
     *    4a) the MEC host is inside the same core network
     *        --> tunnel the packet towards the MEC host
     *    4b) the MEC host is inside another core network
     *        --> tunnel the packet towards the CN gateway
     * 5) otherwise, destination is a UE
     *    5a) the UE is inside the same network
     *        --> tunnel the packet towards its serving BS
     *    5b) the UE is inside another network
     *        --> tunnel the packet towards the CN gateway
     */

    auto tftInfo = datagram->removeTag<TftControlInfo>();
    TrafficFlowTemplateId flowId = tftInfo->getTft();
    Qfi qfi = tftInfo->getQfi();

    EV << "GtpUser::handleFromTrafficFlowFilter - Received a tftMessage with flowId[" << flowId << "] qfi[" << qfi << "]" << endl;

    if (flowId == TFT_REMOVED_DESTINATION) {
        // the destination has been removed from the simulation. Delete datagram
        EV << "GtpUser::handleFromTrafficFlowFilter - Destination has been removed from the simulation. Deleting packet." << endl;
        delete datagram;
        return;
    }

    // If we are on the eNB and the flowId represents the ID of this eNB, forward the packet locally
    if (flowId == TFT_LOCAL_DELIVERY) {
        // local delivery: keep the classified QFI with the packet, the same way
        // handleFromUdp() restores it for tunneled traffic -- SDAP relies on
        // QfiReq being present on the gNB DL path
        datagram->addTagIfAbsent<QfiReq>()->setQfi(qfi);
        send(datagram, "pppGate");  // to the cellular NIC
    }
    else {
        // the packet is ready to be tunneled via GTP to another node in the core network
        L3Address destAddr = datagram->getTag<IpHeaderFieldsTag>()->getDestAddress();

        // create a new GtpUserMessage and encapsulate the datagram within the GtpUserMessage
        auto header = makeShared<GtpUserMsg>();
        header->setTeid(0);
        header->setQfi(qfi);
        header->setChunkLength(B(8));
        auto gtpPacket = new Packet(datagram->getName());
        gtpPacket->insertAtFront(header);
        auto data = datagram->peekData();
        gtpPacket->insertAtBack(data);

        delete datagram;

        L3Address tunnelPeerAddress;
        if (flowId == TFT_EXTERNAL_DESTINATION) { // send to the gateway
            if (gwAddress_.isUnspecified())
                throw cRuntimeError("Packet is destined by TFT to external destination (Internet), but gateway address is not configured");
            EV << "GtpUser::handleFromTrafficFlowFilter - tunneling to " << gwAddress_.str() << endl;
            tunnelPeerAddress = gwAddress_;
        }
        else if (flowId == TFT_MEC_HOST) { // send to a MEC host
            // check if the destination MEC host is within the same core network

            // retrieve the address of the UPF included within the MEC host
            EV << "GtpUser::handleFromTrafficFlowFilter - tunneling to " << destAddr.str() << endl;
            tunnelPeerAddress = binder_->getUpfFromMecHost(destAddr);
        }
        else { // send to a BS
            // check if the destination is within the same core network

            // get the symbolic IP address of the tunnel destination ID
            // then obtain the address via IPvXAddressResolver
            std::string  symbolicName = binder_->getNodeModule(MacNodeId(flowId))->getFullPath();
            EV << "GtpUser::handleFromTrafficFlowFilter - tunneling to " << symbolicName << endl;
            tunnelPeerAddress = L3AddressResolver().resolve(symbolicName.c_str());
        }
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

    // re-create the original IP datagram and send it to the local network
    auto originalPacket = new Packet(pkt->getName());
    auto gtpUserMsg = pkt->popAtFront<GtpUserMsg>();
    originalPacket->insertAtBack(pkt->peekData());
    originalPacket->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&ipProtocolOf(originalPacket));

    // Restore QFI from GTP-U header so SDAP can use it for QFI-to-DRB mapping.
    // Always set the tag, even for QFI 0 (unmarked/default-flow traffic): SDAP
    // relies on QfiReq being present on the gNB DL path, and maps QFI 0 to the
    // default DRB.
    originalPacket->addTagIfAbsent<QfiReq>()->setQfi(gtpUserMsg->getQfi());
    // remove any pending socket indications
    auto sockInd = pkt->removeTagIfPresent<SocketInd>();

    delete pkt;

    if (isBaseStation(ownerType_)) {
        EV << "GtpUser::handleFromUdp - Datagram local delivery to the cellular NIC" << endl;
        send(originalPacket, "pppGate");
    }
    else if (ownerType_ == UPF_MEC) {
        // we are on the MEC, local delivery
        EV << "GtpUser::handleFromUdp - Datagram local delivery to the MEC host" << endl;
        send(originalPacket, "pppGate");
    }
    else if (ownerType_ == PGW || ownerType_ == UPF) {
        // the tunnel does not identify the session (TEID 0), so the destination does
        L3Address destAddr = peekIpHeader(originalPacket)->getDestinationAddress();

        // The IP link of a UE's IPv6 session ends here: link-local-scope traffic (Neighbor
        // Discovery) is answered at this node and must not leak onto the data network
        if (destAddr.getType() == L3Address::IPv6 && isLinkLocalScope(destAddr.toIpv6())) {
            if (!gate("ndOut")->isConnected())
                throw cRuntimeError("GtpUser: link-local IPv6 traffic (destination %s) from a UE arrived, but this node has no Neighbor Discovery responder (see the hasNdResponder parameter)", destAddr.str().c_str());
            send(originalPacket, "ndOut");
            return;
        }

        MacNodeId destId = binder_->getMacNodeId(destAddr);
        if (destId != NODEID_NONE) { // final destination is a UE
            // the UE's tunnel ends at the master of its serving node, as for downlink
            // traffic entering the core network (see TrafficFlowFilter::findTrafficFlow())
            MacNodeId destMaster = binder_->getMasterNodeOrSelf(binder_->getServingNodeOrSelf(destId));

            // check if the destination belongs to the same core network (for multi-operator scenarios)
            std::string gwFullPath = binder_->getNetworkName() + "." + binder_->getModuleByMacNodeId(destMaster)->par("gateway").stdstringValue();
            if (networkNode_->getFullPath() == gwFullPath) {
                // the destination is a Base Station under the same core network as this PGW/UPF,
                // tunnel the packet toward that BS, preserving the QFI of the incoming GTP-U
                tunnelToBaseStation(originalPacket, destMaster, gtpUserMsg->getQfi());
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

    // the UE's tunnel ends at the master of its serving node, the node its downlink enters
    // the radio network at (see TrafficFlowFilter::findTrafficFlow())
    MacNodeId servingNode = binder_->getServingNodeOrSelf(destId);
    if (servingNode == NODEID_NONE) {
        EV_WARN << "GtpUser::handleFromNdResponder - UE " << destId << " is attached to no base station, reply to " << destAddr << " discarded" << endl;
        delete datagram;
        return;
    }
    tunnelToBaseStation(datagram, binder_->getMasterNodeOrSelf(servingNode), Qfi(0));  // the default QoS flow
}

void GtpUser::tunnelToBaseStation(Packet *datagram, MacNodeId bsId, Qfi qfi)
{
    std::string symbolicName = binder_->getNodeModule(bsId)->getFullPath();
    L3Address tunnelPeerAddress = L3AddressResolver().resolve(symbolicName.c_str());
    EV << "GtpUser::tunnelToBaseStation - tunneling to BS " << symbolicName << endl;

    // send the message to the BS through GTP tunneling
    // * create a new GtpUserMessage
    // * encapsulate the datagram within the GtpUserMsg
    auto header = makeShared<GtpUserMsg>();
    header->setTeid(0);
    header->setQfi(qfi);
    header->setChunkLength(B(8));
    auto gtpMsg = new Packet(datagram->getName());
    gtpMsg->insertAtFront(header);
    auto data = datagram->peekData();
    gtpMsg->insertAtBack(data);
    delete datagram;

    EV << "GtpUser::tunnelToBaseStation - Tunneling datagram to " << tunnelPeerAddress.str() << endl;
    socket_.sendTo(gtpMsg, tunnelPeerAddress, tunnelPeerPort_);
}

} //namespace
