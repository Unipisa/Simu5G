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

        binder_.reference(this, "binderModule", true);

        networkIf = getContainingNicModule(this);
        dualConnectivityEnabled_ = networkIf->par("dualConnectivityEnabled").boolValue();
        if (dualConnectivityEnabled_)
            sbTable_ = new SplitBearersTable();
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS) {
        if (nodeType_ == NODEB) {
            cModule *bs = getContainingNode(this);
            nodeId_ = MacNodeId(bs->par("macNodeId").intValue());
        }
        else if (nodeType_ == UE) {
            cModule *ue = getContainingNode(this);
            servingNodeId_ = MacNodeId(ue->par("servingNodeId").intValue());
            nodeId_ = MacNodeId(ue->par("macNodeId").intValue());

            if (ue->hasPar("nrServingNodeId") && ue->par("nrServingNodeId").intValue() != 0) { // register also the NR MacNodeId
                nrServingNodeId_ = MacNodeId(ue->par("nrServingNodeId").intValue());
                nrNodeId_ = MacNodeId(ue->par("nrMacNodeId").intValue());
            }
        }
    }
}

void Ip2Nic::handleMessage(cMessage *msg)
{
    if (msg->getArrivalGate()->isName("upperLayerIn")) {
        if (nodeType_ == NODEB)
            toStackBs(check_and_cast<Packet*>(msg));
        else {
            printControlInfo(check_and_cast<Packet*>(msg));
            toStackUe(check_and_cast<Packet*>(msg));
        }
    }
    else if (msg->getArrivalGate()->isName("stackNic$i")) {
        EV << "Ip2Nic: message from stack: sending up" << endl;
        auto pkt = check_and_cast<Packet *>(msg);
        pkt->removeTagIfPresent<SocketInd>();
        removeAllSimu5GTags(pkt);
        if (nodeType_ == NODEB)
            toIpBs(pkt);
        else
            toIpUe(pkt);
    }
    else {
        throw cRuntimeError("Message received on wrong gate %s", msg->getArrivalGate()->getFullName());
    }
}

void Ip2Nic::setNodeType(std::string s)
{
    nodeType_ = aToNodeType(s);
    EV << "Node type: " << s << endl;
}

void Ip2Nic::toStackUe(Packet *pkt)
{
    EV << "Ip2Nic::fromIpUe - message from IP layer: send to stack: " << pkt->str() << std::endl;
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
    EV << "Ip2Nic::toStackBs - message from IP layer: send to stack" << endl;
    removeAllSimu5GTags(pkt);
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
    if (!pkt->hasTag<IpFlowInd>()) {
        EV << "Ip2Nic::printControlInfo - packet does not have IpFlowInd tag" << endl;
        return;
    }
    EV << "Src IP : " << pkt->getTag<IpFlowInd>()->getSrcAddr() << endl;
    EV << "Dst IP : " << pkt->getTag<IpFlowInd>()->getDstAddr() << endl;
    EV << "ToS : " << pkt->getTag<IpFlowInd>()->getTypeOfService() << endl;
}

bool Ip2Nic::markPacket(inet::Ipv4Address srcAddr, inet::Ipv4Address dstAddr, uint16_t typeOfService, bool& useNR)
{
    // KLUDGE: this is necessary to prevent a runtime error in one of the simulations:
    //
    //    test_numerology, multicell_CBR_UL, ue[9], t=0.001909132428, event #475 (HandoverPacketFilter), #476 (Ip2Nic)
    //    after: ueLteStack=true, ueNrStack=true, servingNodeId=1, nrServingNodeId=2, typeOfService=10 --> useNr = true (ip2nic.cc#319: UE branch, not dualconn("else") )
    //    before: ueLteStack=true, ueNrStack=true,servingNodeId=1, nrServingNodeId=0, typeOfService=10 --> useNr = false, (HandoverPacketFilter.cc#360)
    //
    if (nodeType_ == UE) {
        auto handoverPacketFilter = check_and_cast<HandoverPacketFilter*>(getParentModule()->getSubmodule("handoverPacketFilter"));
        servingNodeId_ = handoverPacketFilter->servingNodeId_;
        nrServingNodeId_ = handoverPacketFilter->nrServingNodeId_;
    }

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
        bool ueLteStack = (binder_->getServingNodeOrSelf(ueId) != NODEID_NONE);
        bool ueNrStack = (binder_->getServingNodeOrSelf(nrUeId) != NODEID_NONE);

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
        bool ueLteStack = (binder_->getServingNodeOrSelf(nodeId_) != NODEID_NONE);
        bool ueNrStack = (binder_->getServingNodeOrSelf(nrNodeId_) != NODEID_NONE);
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
//            servingNodeId_ = binder_->getServingNode(nodeId_);
//            nrServingNodeId_ = binder_->getServingNode(nrNodeId_);
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

Ip2Nic::~Ip2Nic()
{
    if (dualConnectivityEnabled_)
        delete sbTable_;
}

} //namespace
