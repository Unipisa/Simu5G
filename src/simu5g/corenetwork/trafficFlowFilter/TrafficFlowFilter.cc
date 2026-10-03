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

#include "simu5g/corenetwork/trafficFlowFilter/TrafficFlowFilter.h"
#include <inet/common/ProtocolTag_m.h>
#include "simu5g/common/SessionTag_m.h"
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/common/L3Tools.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>

#include "simu5g/common/L3Utils.h"
#include "simu5g/common/QfiTag_m.h"

namespace simu5g {

Define_Module(TrafficFlowFilter);

using namespace inet;
using namespace omnetpp;

void TrafficFlowFilter::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        // reading and setting owner type
        ownerType_ = selectOwnerType(par("ownerType"));
        return;
    }

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_NETWORK_LAYER)
        return;

    // get reference to the binder
    binder_.reference(this, "binderModule", true);

    fastForwarding_ = par("fastForwarding");

    if (ownerType_ == PGW || ownerType_ == UPF)
        gateway_ = binder_->getNetworkName() + "." + getContainingNode(this)->getFullName();
    else
        gateway_ = binder_->getNetworkName() + "." + par("gateway").stdstringValue();

    // mec
    if (isBaseStation(ownerType_)) {
        /*
         * @author Alessandro Noferi
         *
         */
        // obtain the IP address of external MEC applications (if any)

        std::string extAddress = getContainingNode(this)->par("extMeAppsAddress").stringValue();
        if (!extAddress.empty()) {
            std::vector<std::string> extAdd = cStringTokenizer(extAddress.c_str(), "/").asVector();
            if (extAdd.size() != 2) {
                throw cRuntimeError("TrafficFlowFilter::initialize - Bad extMeApps parameter. It must be in the format address/mask");
            }
            meAppsExtAddress_ = inet::L3AddressResolver().resolve(extAdd[0].c_str());
            meAppsExtAddressMask_ = atoi(extAdd[1].c_str());
            EV << "TrafficFlowFilter::initialize - emulation support:  meAppsExtAddress: " << meAppsExtAddress_.str() << "/" << meAppsExtAddressMask_ << endl;
        }
    }

    meHost = par("mecHost").stdstringValue();
    if (isBaseStation(ownerType_) && !meHost.empty()) {
        std::stringstream meHostName;
        meHostName << meHost << ".virtualisationInfrastructure";
        meHost = meHostName.str();
        meHostAddress = inet::L3AddressResolver().resolve(meHost.c_str());

        EV << "TrafficFlowFilter::initialize - meHost: " << meHost << " meHostAddress: " << meHostAddress.str() << endl;
    }
    //end mec
}

void TrafficFlowFilter::setQfiRules(QfiRuleSet&& rules)
{
    Enter_Method_Silent("setQfiRules");
    qfiRules_ = std::move(rules);
}

CoreNodeType TrafficFlowFilter::selectOwnerType(const char *type)
{
    EV << "TrafficFlowFilter::selectOwnerType - setting owner type to " << type << endl;
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
    else
        throw cRuntimeError("TrafficFlowFilter::selectOwnerType - unknown owner type [%s]", type);

    // never gets here
    return ENB;
}

void TrafficFlowFilter::handleMessage(cMessage *msg)
{
    EV << "TrafficFlowFilter::handleMessage - Received Packet:" << endl;
    EV << "name: " << msg->getFullName() << endl;

    Packet *pkt = check_and_cast<Packet *>(msg);

    // the downlink of the session the packet names: a frame of the node's Ethernet
    // session bridge, or a reply of its Neighbor Discovery responder
    if (!isBaseStation(ownerType_) && pkt->findTag<SessionTag>() != nullptr) {
        handleSessionDownlink(pkt);
        return;
    }

    // the payload of a non-IP session (an Unstructured payload, an Ethernet frame),
    // which SDAP delivered up as such at a base station (see Ip2Nic::toIpBs()): no IP
    // header to classify by
    auto protocolTag = pkt->findTag<PacketProtocolTag>();
    if (isBaseStation(ownerType_) && protocolTag != nullptr && isNonIpSessionPayload(protocolTag->getProtocol())) {
        handleNonIpUplink(pkt);
        return;
    }

    // the user-plane entry of the core network (downlink) or of a base station's
    // tunnel (uplink)
    auto ipFields = attachIpHeaderFields(pkt);
    const L3Address& destAddr = ipFields->getDestAddress();
    const L3Address& srcAddr = ipFields->getSrcAddress();

    // TODO check for source and dest port number

    EV << "TrafficFlowFilter::handleMessage - Received datagram : " << pkt->getName() << " - src[" << srcAddr << "] - dest[" << destAddr << "]\n";

    // Link-local IPv6 traffic on the data network link (e.g. the Neighbor Discovery of
    // the router there) is addressed to this node, and is for none of the UEs
    if (!isBaseStation(ownerType_) && destAddr.getType() == L3Address::IPv6 && isLinkLocalScope(destAddr.toIpv6())) {
        EV << "TrafficFlowFilter::handleMessage - link-local traffic on the data network link, consumed" << endl;
        delete pkt;
        return;
    }

    // the downlink of an Unstructured session, on its N6 tunnel
    if (!isBaseStation(ownerType_)) {
        auto it = n6Sessions_.find(destAddr);
        if (it != n6Sessions_.end()) {
            handleN6Downlink(pkt, it->second);
            return;
        }
    }

    // where the datagram goes
    MacNodeId ueNodeId = NODEID_NONE;
    TftOutcome tft = findTrafficFlow(srcAddr, destAddr, ueNodeId);

    // add control info to the normal IP datagram. This info will be read by the GTP-U application
    auto tftInfo = pkt->addTag<TftControlInfo>();
    tftInfo->setTft(tft);
    tftInfo->setUeNodeId(ueNodeId);

    // QFI assignment. An uplink packet that SDAP already attributed to a QoS flow
    // (the QfiInd tag, carrying the QFI the UE classified it with) keeps that QFI on
    // its way into the core network, rather than being re-classified by this node's
    // rules -- the two rule sets can disagree, and the UE's classification is the
    // flow's identity.
    //
    // At a core-network tunnel entry the rules are the ones the bearer configurator
    // delivered (see its dlQfiRules parameter), modeling the packet detection and
    // QoS enforcement rules (PDR/QER) that the SMF installs into a UPF over N4 at
    // session setup. A base station is no such enforcement point, so unattributed
    // traffic entering the tunnel there (uplink from a stack without SDAP) is
    // classified by the built-in DSCP-as-QFI residual instead.
    Qfi qfi = QFI_NONE;
    if (auto qfiInd = pkt->findTag<QfiInd>())
        qfi = qfiInd->getQfi();
    if (qfi == QFI_NONE)
        qfi = isBaseStation(ownerType_) ? Qfi(dscpOf(ipFields->getTos())) : qfiRules_.classify(pkt, dscpOf(ipFields->getTos()));
    if (qfi == QFI_NONE)
        qfi = Qfi(0);   // traffic no rule covers belongs to the default flow
    tftInfo->setQfi(qfi);

    EV << "TrafficFlowFilter::handleMessage - setting tft=" << tft << " ueNodeId=" << ueNodeId << " qfi=" << qfi << endl;

    // send the datagram to the GTP-U module
    send(pkt, "gtpUserGateOut");
}

void TrafficFlowFilter::handleNonIpUplink(Packet *pkt)
{
    // MEC steering and local delivery go by the IP destination, which the payload has
    // none of: the payload goes to the session's anchor
    ASSERT(isBaseStation(ownerType_));
    auto tftInfo = pkt->addTag<TftControlInfo>();
    tftInfo->setTft(TFT_EXTERNAL_DESTINATION);
    tftInfo->setUeNodeId(NODEID_NONE);
    auto qfiInd = pkt->findTag<QfiInd>();
    tftInfo->setQfi(qfiInd != nullptr ? qfiInd->getQfi() : Qfi(0));
    EV << "TrafficFlowFilter::handleNonIpUplink - " << pkt->getName() << " goes to the anchor, qfi=" << tftInfo->getQfi() << endl;
    send(pkt, "gtpUserGateOut");
}

void TrafficFlowFilter::handleN6Downlink(Packet *pkt, const N6Session& n6Session)
{
    // the datagram must be the N6 tunnel's: UDP to the port the UPF receives on
    const Protocol& ipProtocol = ipProtocolOf(pkt);
    auto ipHeader = removeNetworkProtocolHeader(pkt, ipProtocol);
    if (ipHeader->getProtocol() != &Protocol::udp)
        throw cRuntimeError("TrafficFlowFilter: a %s datagram arrived for %s, the N6 address of an Unstructured session, whose tunnel is UDP",
                ipHeader->getProtocol() != nullptr ? ipHeader->getProtocol()->getName() : "non-UDP", n6Session.tunnel.sessionAddress.str().c_str());
    auto udpHeader = pkt->removeAtFront<UdpHeader>();
    if (udpHeader->getDestinationPort() != n6Session.tunnel.localPort)
        throw cRuntimeError("TrafficFlowFilter: a UDP datagram arrived for %s, the N6 address of an Unstructured session, on port %d, "
                "but the session's N6 tunnel receives on port %d", n6Session.tunnel.sessionAddress.str().c_str(),
                (int)udpHeader->getDestinationPort(), n6Session.tunnel.localPort);
    EV << "TrafficFlowFilter::handleN6Downlink - " << pkt->getName() << " is the downlink of " << n6Session.session << endl;

    // the payload of the session, on its one QoS flow; no rule is evaluated
    pkt->removeTag<IpHeaderFieldsTag>();
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::unstructured);
    auto tftInfo = pkt->addTag<TftControlInfo>();
    tftInfo->setTft(TFT_PDU_SESSION);
    tftInfo->setUeNodeId(n6Session.session.lteNodeId);
    tftInfo->setQfi(Qfi(0));
    send(pkt, "gtpUserGateOut");
}

void TrafficFlowFilter::handleSessionDownlink(Packet *pkt)
{
    ASSERT(!isBaseStation(ownerType_));
    auto session = pkt->removeTag<SessionTag>();
    Qfi qfi = Qfi(0);   // the Neighbor Discovery of the session's link goes on the default flow
    if (pkt->getTag<PacketProtocolTag>()->getProtocol() == &Protocol::ethernetMac) {
        ASSERT(ownerType_ == UPF);
        qfi = qfiRules_.classifyFrame(pkt);
        if (qfi == QFI_NONE)
            qfi = Qfi(0);   // frames no rule covers belong to the default flow
    }
    auto tftInfo = pkt->addTag<TftControlInfo>();
    tftInfo->setTft(TFT_PDU_SESSION);
    tftInfo->setUeNodeId(session->getLteNodeId());
    tftInfo->setQfi(qfi);
    tftInfo->setSessionNamed(true);
    EV << "TrafficFlowFilter::handleSessionDownlink - " << pkt->getName() << " of the session of UE " << session->getLteNodeId() << ", qfi=" << qfi << endl;
    send(pkt, "gtpUserGateOut");
}

void TrafficFlowFilter::addN6Tunnel(const SessionRef& session, const N6Tunnel& tunnel)
{
    Enter_Method_Silent("addN6Tunnel");
    ASSERT(ownerType_ == UPF);
    if (!n6Sessions_.emplace(tunnel.sessionAddress, N6Session{session, tunnel}).second)
        throw cRuntimeError("TrafficFlowFilter: the N6 address %s is already that of another session", tunnel.sessionAddress.str().c_str());
}

void TrafficFlowFilter::removeN6Tunnel(const SessionRef& session)
{
    Enter_Method_Silent("removeN6Tunnel");
    for (auto it = n6Sessions_.begin(); it != n6Sessions_.end(); ++it) {
        if (it->second.session.lteNodeId == session.lteNodeId && it->second.session.id == session.id) {
            n6Sessions_.erase(it);
            return;
        }
    }
}

TftOutcome TrafficFlowFilter::findTrafficFlow(const L3Address& srcAddress, const L3Address& destAddress, MacNodeId& ueNodeId)
{
    // check whether the destination address is a (simulated) MEC host's address
    if (binder_->isMecHost(destAddress)) {
        // check if the destination belongs to another core network (for multi-operator scenarios)
        std::string destGw = binder_->getNetworkName() + "." + CHK(inet::L3AddressResolver().findHostWithAddress(destAddress))->par("gateway").stdstringValue();
        if (gateway_ != destGw) {
            // the destination is a MEC host under a different core network, send the packet to the gateway
            return TFT_EXTERNAL_DESTINATION;
        }

        EV << "TrafficFlowFilter::findTrafficFlow - returning flowId (TFT_MEC_HOST) for tunneling to " << destAddress.str() << endl;
        return TFT_MEC_HOST;
    }
    // emulation mode
    else if (!meAppsExtAddress_.isUnspecified() && destAddress.matches(meAppsExtAddress_, meAppsExtAddressMask_)) {
        // the destination is a MecApplication running outside the simulator, forward to meHost (it has forwarding enabled)
        EV << "TrafficFlowFilter::findTrafficFlow - returning flowId (TFT_MEC_HOST) for tunneling to " << destAddress.str() << " (external) " << endl;
        return TFT_MEC_HOST;
    }

    MacNodeId destId = binder_->getMacNodeId(destAddress);
    destId = (destId != NODEID_NONE) ? destId : binder_->getNrMacNodeId(destAddress);
    if (destId == NODEID_NONE) {
        EV << "TrafficFlowFilter::findTrafficFlow - destination " << destAddress.str() << " is not a UE. ";
        if (ownerType_ == UPF || ownerType_ == PGW) {
            EV << "Remove packet from the simulation." << endl;
            return TFT_REMOVED_DESTINATION;   // the destination UE has been removed from the simulation
        }
        else { // BS or MEC
            EV << "Forward packet to the gateway." << endl;
            return TFT_EXTERNAL_DESTINATION;   // the destination might be outside the cellular network, send the packet to the gateway
        }
    }

    if (!isBaseStation(ownerType_)) {
        // MEC host or PGW/UPF: the downlink of the destination UE's session, whose
        // tunnel the GTP-U endpoint knows (or knows it has none, see GtpUser)
        EV << "TrafficFlowFilter::findTrafficFlow - destination " << destAddress.str() << " is UE " << destId << endl;
        ueNodeId = destId;
        return TFT_PDU_SESSION;
    }

    MacNodeId destBS = binder_->getServingNodeOrSelf(destId);
    if (destBS == NODEID_NONE) {
        EV << "TrafficFlowFilter::findTrafficFlow - destination " << destAddress.str() << " is a UE [" << destId << "] not attached to any BS. Remove packet from the simulation." << endl;
        return TFT_REMOVED_DESTINATION;   // the destination UE is not attached to any nodeB
    }

    // the serving node for the UE might be a secondary node in case of NR Dual Connectivity
    // obtains the master node, if any (the function returns destEnb if it is a master already)
    MacNodeId destMaster = binder_->getMasterNodeOrSelf(destBS);
    MacNodeId srcMaster = binder_->getServingNodeOrSelf(binder_->getMacNodeId(srcAddress));

    if (fastForwarding_ && srcMaster == destMaster)
        return TFT_LOCAL_DELIVERY;                       // local delivery

    return TFT_EXTERNAL_DESTINATION;   // send the packet to the PGW/UPF. It will forward the packet to the correct BS
                                      // TODO if the BS is within the same core network, there should be a direct tunnel to
                                      //      it without going through the gateway (for now, this is not implemented as it
                                      //      may cause packets being transmitted via the X2
}

} //namespace
