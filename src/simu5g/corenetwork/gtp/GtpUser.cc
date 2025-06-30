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
#include "corenetwork/gtp/GtpUser.h"
#include "corenetwork/trafficFlowFilter/TftControlInfo_m.h"
#include <iostream>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
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

    networkNode_ = getContainingNode(this);

    ownerType_ = selectOwnerType(networkNode_->par("nodeType"));

    // find the address of the core network gateway
    if (ownerType_ != PGW && ownerType_ != UPF) {
        // check if this is a gNB connected as a secondary node
        bool connectedBS = isBaseStation(ownerType_) && networkNode_->gate("ppp$o")->isConnected();

        if (connectedBS || ownerType_ == UPF_MEC) {
            if (par("gateway").isEmptyString())
                throw cRuntimeError("The required 'gateway' parameter is empty.");
            std::string gateway = binder_->getNetworkName() + "." + par("gateway").stdstringValue();
            gwAddress_ = L3AddressResolver().resolve(gateway.c_str());
        }
    }

    if (isBaseStation(ownerType_))
        myMacNodeID = MacNodeId(networkNode_->par("macNodeId").intValue());
    else
        myMacNodeID = NODEID_NONE;

    ie_ = detectInterface();
}

NetworkInterface *GtpUser::detectInterface()
{
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    const char *interfaceName = par("ipOutInterface");
    NetworkInterface *ie = nullptr;

    if (strlen(interfaceName) > 0) {
        ie = ift->findInterfaceByName(interfaceName);
        if (ie == nullptr)
            throw cRuntimeError("Interface \"%s\" does not exist", interfaceName);
    }

    return ie;
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

    throw cRuntimeError("GtpUser::selectOwnerType - unknown owner type [%s]. Aborting...", type);

    // you should not be here
    return ENB;
}

void GtpUser::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("trafficFlowFilterGate")) {
        EV << "GtpUser::handleMessage - message from trafficFlowFilter" << endl;

        // forward the encapsulated IPv4 datagram
        handleFromTrafficFlowFilter(check_and_cast<Packet *>(msg));
    }
    else if (msg->arrivedOn("socketIn")) {
        EV << "GtpUser::handleMessage - message from UDP layer" << endl;
        Packet *packet = check_and_cast<Packet *>(msg);
        PacketPrinter printer; // turns packets into human-readable strings
        printer.printPacket(EV, packet); // print to standard output

        handleFromUdp(packet);
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

    EV << "GtpUser::handleFromTrafficFlowFilter - Received a tftMessage with flowId[" << flowId << "]" << endl;

    if (flowId == -2) {
        // the destination has been removed from the simulation. Delete datagram
        EV << "GtpUser::handleFromTrafficFlowFilter - Destination has been removed from the simulation. Deleting packet." << endl;
        delete datagram;
        return;
    }

    // If we are on the eNB and the flowId represents the ID of this eNB, forward the packet locally
    if (flowId == 0) {
        // local delivery
        send(datagram, "pppGate");
    }
    else {
        // the packet is ready to be tunneled via GTP to another node in the core network
        const auto& hdr = datagram->peekAtFront<Ipv4Header>();
        const Ipv4Address& destAddr = hdr->getDestAddress();

        // create a new GtpUserMessage and encapsulate the datagram within the GtpUserMessage
        auto header = makeShared<GtpUserMsg>();
        header->setTeid(0);
        header->setChunkLength(B(8));
        auto gtpPacket = new Packet(datagram->getName());
        gtpPacket->insertAtFront(header);
        auto data = datagram->peekData();
        gtpPacket->insertAtBack(data);

        delete datagram;

        L3Address tunnelPeerAddress;
        if (flowId == -1) { // send to the gateway
            EV << "GtpUser::handleFromTrafficFlowFilter - tunneling to " << gwAddress_.str() << endl;
            tunnelPeerAddress = gwAddress_;
        }
        else if (flowId == -3) { // send to a MEC host
            // check if the destination MEC host is within the same core network

            // retrieve the address of the UPF included within the MEC host
            EV << "GtpUser::handleFromTrafficFlowFilter - tunneling to " << destAddr.str() << endl;
            tunnelPeerAddress = binder_->getUpfFromMecHost(inet::L3Address(destAddr));
        }
        else { // send to a BS
            // check if the destination is within the same core network

            // get the symbolic IP address of the tunnel destination ID
            // then obtain the address via IPvXAddressResolver
            const char *symbolicName = binder_->getModuleNameByMacNodeId(MacNodeId(flowId));
            EV << "GtpUser::handleFromTrafficFlowFilter - tunneling to " << symbolicName << endl;
            tunnelPeerAddress = L3AddressResolver().resolve(symbolicName);
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
    originalPacket->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
    // remove any pending socket indications
    auto sockInd = pkt->removeTagIfPresent<SocketInd>();

    delete pkt;

    const auto& hdr = originalPacket->peekAtFront<Ipv4Header>();
    const Ipv4Address& destAddr = hdr->getDestAddress();

    if (isBaseStation(ownerType_)) {
        // add Interface-Request for cellular NIC
        if (ie_ != nullptr)
            originalPacket->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ie_->getInterfaceId());

        EV << "GtpUser::handleFromUdp - Datagram local delivery to " << destAddr.str() << endl;
        // local delivery
        send(originalPacket, "pppGate");
    }
    else if (ownerType_ == UPF_MEC) {
        // we are on the MEC, local delivery
        EV << "GtpUser::handleFromUdp - Datagram local delivery to " << destAddr.str() << endl;
        send(originalPacket, "pppGate");
    }
    else if (ownerType_ == PGW || ownerType_ == UPF) {
        MacNodeId destId = binder_->getMacNodeId(destAddr);
        if (destId != NODEID_NONE) { // final destination is a UE
            MacNodeId destMaster = binder_->getNextHop(destId);

            // check if the destination belongs to the same core network (for multi-operator scenarios)
            std::string gwFullPath = binder_->getNetworkName() + "." + binder_->getModuleByMacNodeId(destMaster)->par("gateway").stdstringValue();
            if (networkNode_->getFullPath() == gwFullPath) {
                // the destination is a Base Station under the same core network as this PGW/UPF,
                // tunnel the packet toward that BS
                const char *symbolicName = binder_->getModuleNameByMacNodeId(destMaster);
                L3Address tunnelPeerAddress = L3AddressResolver().resolve(symbolicName);
                EV << "GtpUser::handleFromUdp - tunneling to BS " << symbolicName << endl;

                // send the message to the BS through GTP tunneling
                // * create a new GtpUserMessage
                // * encapsulate the datagram within the GtpUserMsg
                auto header = makeShared<GtpUserMsg>();
                header->setTeid(0);
                header->setChunkLength(B(8));
                auto gtpMsg = new Packet(originalPacket->getName());
                gtpMsg->insertAtFront(header);
                auto data = originalPacket->peekData();
                gtpMsg->insertAtBack(data);
                delete originalPacket;

                // create a new GtpUserMessage
                EV << "GtpUser::handleFromUdp - Tunneling datagram to " << tunnelPeerAddress.str() << ", final destination[" << destAddr.str() << "]" << endl;
                socket_.sendTo(gtpMsg, tunnelPeerAddress, tunnelPeerPort_);
                return;
            }
        }

        // destination is outside the radio network
        EV << "GtpUser::handleFromUdp - Sending datagram outside the radio network, destination[" << destAddr.str() << "]" << endl;
        send(originalPacket, "pppGate");
    }
}

} //namespace

