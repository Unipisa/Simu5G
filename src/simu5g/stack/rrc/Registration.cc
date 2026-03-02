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
#include <inet/networklayer/ipv4/IIpv4RoutingTable.h>
#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>
#include <inet/networklayer/ipv4/Ipv4Route.h>
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

        cModule *containingNode = inet::getContainingNode(this);
        MacNodeId nodeId = MacNodeId(containingNode->par("macNodeId").intValue());
        nodeType = getNodeTypeById(nodeId);
        if (nodeType == UE) {
            lteNodeId = nodeId;
            if (containingNode->hasPar("nrMacNodeId"))
                nrNodeId = MacNodeId(containingNode->par("nrMacNodeId").intValue());
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
    else if (stage == inet::INITSTAGE_STATIC_ROUTING) {
        if (nodeType == UE) {
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
}

void Registration::registerMulticastGroups()
{
    // get all the multicast addresses where the node is enrolled
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    NetworkInterface *iface = ift->findInterfaceByName(par("interfaceName").stdstringValue().c_str());
    unsigned int numOfAddresses = iface->getProtocolData<Ipv4InterfaceData>()->getNumOfJoinedMulticastGroups();

    for (unsigned int i = 0; i < numOfAddresses; ++i) {
        Ipv4Address addr = iface->getProtocolData<Ipv4InterfaceData>()->getJoinedMulticastGroup(i);
        MacNodeId multicastDestId = binder->getOrAssignDestIdForMulticastAddress(addr);
        // register in the LTE and also the NR stack, if any
        binder->joinMulticastGroup(lteNodeId, multicastDestId);
        if (nrNodeId != NODEID_NONE)
            binder->joinMulticastGroup(nrNodeId, multicastDestId);
    }
}

void Registration::handleMessage(cMessage *msg)
{
    throw cRuntimeError("This module does not process messages");
}

} // namespace simu5g
