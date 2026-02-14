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

#include <inet/networklayer/ipv4/IIpv4RoutingTable.h>
#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>
#include <inet/networklayer/ipv4/Ipv4Route.h>
#include "simu5g/stack/rrc/Rrc.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/rlc/um/LteRlcUm.h"
#include "simu5g/stack/rlc/am/LteRlcAm.h"
#include "simu5g/stack/pdcp/LtePdcp.h"
#include "simu5g/common/InitStages.h"

namespace simu5g {

Define_Module(Rrc);

void Rrc::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder.reference(this, "binderModule", true);

        pdcpModule.reference(this, "pdcpModule", true);
        rlcUmModule.reference(this, "rlcUmModule", true);
        nrRlcUmModule.reference(this, "nrRlcUmModule", false);
        macModule.reference(this, "macModule", true);
        nrMacModule.reference(this, "nrMacModule", false);

        cModule *containingNode = getContainingNode(this);
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
            cModule *bs = getContainingNode(this);
            bool isNr = nrNodeId != NODEID_NONE;
            MacNodeId nodeId = isNr ? nrNodeId : lteNodeId;
            binder->registerNode(nodeId, bs, NODEB, isNr);

            // display node ID above node icon
            bs->getDisplayString().setTagArg("t", 0, opp_stringf("nodeId=%d", nodeId).c_str());
        }
        else if (nodeType == UE) {
            cModule *ue = getContainingNode(this);
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
            cModule *bs = getContainingNode(this);
            bool isNr = nrNodeId != NODEID_NONE;
            MacNodeId nodeId = isNr ? nrNodeId : lteNodeId;
            MacNodeId masterId = MacNodeId(bs->par("masterId").intValue());
            binder->registerMasterNode(masterId, nodeId);  // note: even if masterId == NODEID_NONE!
        }
        if (nodeType == UE) {
            cModule *ue = getContainingNode(this);
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

void Rrc::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH) {
        if (lteNodeId != NODEID_NONE)
            binder->unregisterNode(lteNodeId);
        if (nrNodeId != NODEID_NONE)
            binder->unregisterNode(nrNodeId);
    }
}

void Rrc::registerInterface()
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

void Rrc::registerMulticastGroups()
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


void Rrc::handleMessage(cMessage *msg)
{
    throw cRuntimeError("This module does not process messages");
}

void Rrc::createIncomingConnection(FlowControlInfo *lteInfo, bool withPdcp)
{
    Enter_Method_Silent("createIncomingConnection()");

    EV << "Rrc::createIncomingConnection - " << " srcId=" << lteInfo->getSourceId() << " destId=" << lteInfo->getDestId()
        << " groupId=" << lteInfo->getMulticastGroupId() << " drbId=" << lteInfo->getDrbId()
        << " direction=" << dirToA(lteInfo->getDirection())
        << " withPdcp=" << (withPdcp ? "yes" : "no") << endl;

    ASSERT(lteInfo->getDestId() == getLteNodeId() || lteInfo->getDestId() == getNrNodeId() || lteInfo->getMulticastGroupId() != NODEID_NONE);

    // Create MAC incoming connection
    FlowDescriptor desc = FlowDescriptor::fromFlowControlInfo(*lteInfo);
    MacNodeId senderId = desc.getSourceId();
    auto mac = (nodeType==UE && isNrUe(lteInfo->getDestId())) ? nrMacModule.get() : macModule.get(); //TODO FIXME! DOES NOT WORK FOR MULTICAST!!!!!
    LogicalCid lcid = mac->drbIdToLcid(desc.getDrbId());
    MacCid cid = MacCid(senderId, lcid);
    mac->createIncomingConnection(cid, desc);

    // RLC: only UM works
    MacNodeId nodeIdForRlc = (lteInfo->getDirection() == DL) ? lteInfo->getDestId() : lteInfo->getSourceId();
    DrbKey rlcId = DrbKey(nodeIdForRlc, lteInfo->getDrbId());
    auto rlcUm = (nodeType==UE && isNrUe(lteInfo->getDestId())) ? nrRlcUmModule.get() : rlcUmModule.get(); //TODO FIXME! DOES NOT WORK FOR MULTICAST!!!!!
    rlcUm->createRxBuffer(rlcId, lteInfo);

    // PDCP is not needed on Secondary nodes
    if (withPdcp) {
        DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());
        pdcpModule->createRxEntity(id);
    }
}

void Rrc::createOutgoingConnection(FlowControlInfo *lteInfo, bool withPdcp)
{
    Enter_Method_Silent("createOutgoingConnection()");

    EV << "Rrc::createOutgoingConnection - " << " srcId=" << lteInfo->getSourceId() << " destId=" << lteInfo->getDestId()
        << " groupId=" << lteInfo->getMulticastGroupId() << " drbId=" << lteInfo->getDrbId()
        << " direction=" << dirToA(lteInfo->getDirection())
        << " withPdcp=" << (withPdcp ? "yes" : "no") << endl;

    ASSERT(lteInfo->getSourceId() == getLteNodeId() || lteInfo->getSourceId() == getNrNodeId());

    // Create MAC outgoing connection
    FlowDescriptor desc = FlowDescriptor::fromFlowControlInfo(*lteInfo);
    MacNodeId destId = desc.getDestId();
    auto mac = (nodeType==UE && isNrUe(lteInfo->getSourceId())) ? nrMacModule.get() : macModule.get();
    LogicalCid lcid = mac->drbIdToLcid(desc.getDrbId());
    MacCid cid = MacCid(destId, lcid);
    mac->createOutgoingConnection(cid, desc);

    // RLC: only UM works
    DrbKey rlcId = ctrlInfoToNodeDrbId(lteInfo);
    auto rlcUm = (nodeType==UE && isNrUe(lteInfo->getSourceId())) ? nrRlcUmModule.get() : rlcUmModule.get();
    rlcUm->createTxBuffer(rlcId, lteInfo);

    // PDCP is not needed on Secondary nodes
    if (withPdcp) {
        DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());
        pdcpModule->createTxEntity(id);
    }
}

} // namespace simu5g
