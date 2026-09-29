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

#include <inet/common/ModuleAccess.h>
#include <inet/common/Simsignals.h>
#include <inet/common/stlutils.h>
#include <inet/networklayer/ipv4/IIpv4RoutingTable.h>
#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>
#include <inet/networklayer/ipv4/Ipv4Route.h>
#include <inet/networklayer/ipv6/Ipv6InterfaceData.h>
#include "simu5g/stack/rrc/Registration.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/InitStages.h"

namespace simu5g {

using namespace inet;

Define_Module(Registration);

void Registration::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder.reference(this, "binderModule", true);
        connectionControl.reference(this, "connectionControlModule", true);

        cModule *containingNode = inet::getContainingNode(this);
        MacNodeId nodeId = MacNodeId(containingNode->par("macNodeId").intValue());
        nodeType = getNodeTypeById(nodeId);
        if (nodeType == UE) {
            lteNodeId = nodeId;
            if (containingNode->hasPar("nrMacNodeId"))
                nrNodeId = MacNodeId(containingNode->par("nrMacNodeId").intValue());
            sessionType_ = aToSessionType(par("sessionType").stdstringValue());
        }
        if (nodeType == NODEB) {
            bool isNr = containingNode->par("nodeType").stdstringValue() == "GNODEB";
            (isNr ? nrNodeId : lteNodeId) = nodeId;
        }
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS) {
        if (nodeType == NODEB) {
            cModule *bs = inet::getContainingNode(this);
            bool isNr = nrNodeId != NODEID_NONE;
            MacNodeId nodeId = isNr ? nrNodeId : lteNodeId;
            binder->registerNode(nodeId, bs, NODEB, isNr);

            // display node ID above node icon
            bs->getDisplayString().setTagArg("t", 0, opp_stringf("nodeId=%d", nodeId).c_str());
        }
        else if (nodeType == UE) {
            cModule *ue = inet::getContainingNode(this);
            binder->registerNode(lteNodeId, ue, UE, false);
            ue->getDisplayString().setTagArg("t", 0, opp_stringf("nodeId=%d", lteNodeId).c_str());

            if (nrNodeId != NODEID_NONE) {
                binder->registerNode(nrNodeId, ue, UE, true);
                ue->getDisplayString().setTagArg("t", 0, opp_stringf("nodeId=%d/%d", lteNodeId, nrNodeId).c_str());
            }
        }

        registerInterface();
    }
    else if (stage == INITSTAGE_SIMU5G_NODE_RELATIONSHIPS) {
        if (nodeType == NODEB) {
            cModule *bs = inet::getContainingNode(this);
            bool isNr = nrNodeId != NODEID_NONE;
            MacNodeId nodeId = isNr ? nrNodeId : lteNodeId;
            MacNodeId masterId = MacNodeId(bs->par("masterId").intValue());
            binder->registerMasterNode(masterId, nodeId);  // note: even if masterId == NODEID_NONE!
        }
        if (nodeType == UE) {
            cModule *ue = inet::getContainingNode(this);
            MacNodeId servingNodeId = MacNodeId(ue->par("servingNodeId").intValue());
            binder->registerServingNode(servingNodeId, lteNodeId);
            if (nrNodeId != NODEID_NONE) {
                MacNodeId nrServingNodeId = MacNodeId(ue->par("nrServingNodeId").intValue());
                binder->registerServingNode(nrServingNodeId, nrNodeId);
            }
        }
    }
    else if (stage == inet::INITSTAGE_NETWORK_LAYER) {
        if (nodeType == UE) {
            // The Binder's address-to-node-id mapping is how the network side finds the
            // UE a downlink packet is for
            registerAddresses();
            cModule *ue = inet::getContainingNode(this);
            ue->subscribe(interfaceIpv4ConfigChangedSignal, this);
            ue->subscribe(interfaceIpv6ConfigChangedSignal, this);

            // emulation: the external host behind the UE is reached through it
            const char *extHostAddress = ue->par("extHostAddress").stringValue();
            if (strcmp(extHostAddress, "") != 0) {
                binder->setMacNodeId(Ipv4Address(extHostAddress), lteNodeId);
                if (nrNodeId != NODEID_NONE)
                    binder->setMacNodeId(Ipv4Address(extHostAddress), nrNodeId);
            }
        }
    }
    else if (stage == inet::INITSTAGE_STATIC_ROUTING) {
        if (nodeType == UE) {
            // if the UE has been created dynamically, we need to manually add a default route having our cellular interface as output interface
            // otherwise we are not able to reach devices outside the cellular network
            // (An IPv6 UE gets its default route from the UPF's Router Advertisement.)
            // The NIC of a non-IP session carries no IP traffic.
            IIpv4RoutingTable *irt = findModuleFromPar<IIpv4RoutingTable>(par("routingTableModule"), this);
            if (NOW > 0 && irt != nullptr && isIpSessionType(sessionType_)) {
                /**
                 * TODO: might need a bit more care, if the interface has changed, the query might, too
                 */
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

void Registration::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH) {
        if (lteNodeId != NODEID_NONE)
            binder->unregisterNode(lteNodeId);
        if (nrNodeId != NODEID_NONE)
            binder->unregisterNode(nrNodeId);
    }
}

void Registration::registerInterface()
{
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (!ift)
        return;

    networkIf = inet::getContainingNicModule(this);
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

    // The NIC of an Ethernet session is an Ethernet port of the UE's link layer (see
    // ~LteUe): broadcast, not point-to-point, with a MAC address of its own (3GPP assigns
    // the UE none; the NIC needs one as any Ethernet port does)
    if (nodeType == UE && sessionType_ == ETHERNET) {
        networkIf->setProtocol(&Protocol::ethernetMac);
        networkIf->setBroadcast(true);
        networkIf->setPointToPoint(false);
        networkIf->setMacAddress(MacAddress::generateAutoAddress());
    }
}

void Registration::registerAddresses()
{
    std::set<L3Address> addresses;
    if (auto ipv4Data = networkIf->findProtocolData<Ipv4InterfaceData>())
        if (!ipv4Data->getIPAddress().isUnspecified())
            addresses.insert(ipv4Data->getIPAddress());
    if (auto ipv6Data = networkIf->findProtocolData<Ipv6InterfaceData>())
        for (int i = 0; i < ipv6Data->getNumAddresses(); i++)
            if (!ipv6Data->isTentativeAddress(i))
                addresses.insert(ipv6Data->getAddress(i));

    for (const auto& address : registeredAddresses) {
        if (!contains(addresses, address)) {
            EV_INFO << "Registration: address " << address << " no longer maps to this UE" << endl;
            binder->unsetMacNodeId(address, lteNodeId);
            if (nrNodeId != NODEID_NONE)
                binder->unsetMacNodeId(address, nrNodeId);
        }
    }
    for (const auto& address : addresses) {
        if (!contains(registeredAddresses, address)) {
            EV_INFO << "Registration: address " << address << " maps to this UE" << endl;
            binder->setMacNodeId(address, lteNodeId);
            if (nrNodeId != NODEID_NONE)
                binder->setMacNodeId(address, nrNodeId);
        }
    }
    registeredAddresses = addresses;
}

void Registration::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    Enter_Method("%s", cComponent::getSignalName(signalID));
    auto change = check_and_cast<const NetworkInterfaceChangeDetails *>(obj);
    if (change->getNetworkInterface() == networkIf)
        registerAddresses();
}

void Registration::registerMulticastGroups()
{
    // get all the multicast addresses where the node is enrolled
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    NetworkInterface *iface = ift->findInterfaceByName(par("interfaceName").stdstringValue().c_str());

    // IPv4 groups only: multicast on the UE side is D2D groupcast, which is IPv4-only
    auto ipv4Data = iface->findProtocolData<Ipv4InterfaceData>();
    if (ipv4Data == nullptr)
        return;
    unsigned int numOfAddresses = ipv4Data->getNumOfJoinedMulticastGroups();

    for (unsigned int i = 0; i < numOfAddresses; ++i) {
        Ipv4Address addr = ipv4Data->getJoinedMulticastGroup(i);
        MacNodeId multicastDestId = binder->getOrAssignDestIdForMulticastAddress(addr);
        // register in the LTE and also the NR stack, if any
        binder->joinMulticastGroup(lteNodeId, multicastDestId);
        connectionControl->multicastGroupJoined(lteNodeId, multicastDestId);
        if (nrNodeId != NODEID_NONE) {
            binder->joinMulticastGroup(nrNodeId, multicastDestId);
            connectionControl->multicastGroupJoined(nrNodeId, multicastDestId);
        }
    }
}

void Registration::handleMessage(cMessage *msg)
{
    throw cRuntimeError("This module does not process messages");
}

} // namespace simu5g
