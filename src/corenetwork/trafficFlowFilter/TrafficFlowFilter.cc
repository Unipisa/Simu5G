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

#include "corenetwork/trafficFlowFilter/TrafficFlowFilter.h"
#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>

Define_Module(TrafficFlowFilter);

using namespace inet;
using namespace omnetpp;

void TrafficFlowFilter::initialize(int stage)
{
    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_NETWORK_LAYER)
        return;

    // get reference to the binder
    binder_ = getBinder();

    fastForwarding_ = par("fastForwarding");

    // reading and setting owner type
    ownerType_ = selectOwnerType(par("ownerType"));
    if (ownerType_ == PGW || ownerType_ == UPF)
    {
        gateway_ = getParentModule()->getFullName();
    }
    else if(getParentModule()->hasPar("gateway") || getParentModule()->getParentModule()->hasPar("gateway"))
    {
        gateway_ = getAncestorPar("gateway").stringValue();
    }

    // mec
    if(isBaseStation(ownerType_))
    {
        /*
          * @author Alessandro Noferi
          *
          */
        // obtain the IP address of externel MEC applications (if any)

        std::string extAddress = getAncestorPar("extMeAppsAddress").stringValue();
        if(strcmp(extAddress.c_str(), "") != 0)
        {
            std::vector<std::string> extAdd =  cStringTokenizer(extAddress.c_str(), "/").asVector();
            if(extAdd.size() != 2){
                throw cRuntimeError("TrafficFlowFilter::initialize - Bad extMeApps parameter. It must be like address/mask");
            }
            meAppsExtAddress_ = inet::L3AddressResolver().resolve(extAdd[0].c_str());
            meAppsExtAddressMask_ = atoi(extAdd[1].c_str());
            EV << "TrafficFlowFilter::initialize - emulation support:  meAppsExtAddres: " << meAppsExtAddress_.str()<<"/"<< meAppsExtAddressMask_<< endl;
        }
    }

    if(getParentModule()->hasPar("mecHost")){

        meHost = getParentModule()->par("mecHost").stringValue();
        if(isBaseStation(ownerType_) &&  strcmp(meHost.c_str(), ""))
        {
            std::stringstream meHostName;
            meHostName << meHost.c_str() << ".virtualisationInfrastructure";
            meHost = meHostName.str();
            meHostAddress = inet::L3AddressResolver().resolve(meHost.c_str());

            EV << "TrafficFlowFilter::initialize - meHost: " << meHost << " meHostAddress: " << meHostAddress.str() << endl;
        }
    }
    //end mec

    // register service processing IP-packets on the LTE Uu Link
    auto gateIn = gate("internetFilterGateIn");
    registerProtocol(LteProtocol::ipv4uu, gateIn, SP_INDICATION);
    registerProtocol(LteProtocol::ipv4uu, gateIn, SP_CONFIRM);
}

CoreNodeType TrafficFlowFilter::selectOwnerType(const char * type)
{
    EV << "TrafficFlowFilter::selectOwnerType - setting owner type to " << type << endl;
    if(strcmp(type,"ENODEB") == 0)
        return ENB;
    else if(strcmp(type,"GNODEB") == 0)
        return GNB;
    else if(strcmp(type,"PGW") == 0)
        return PGW;
    else if(strcmp(type,"UPF") == 0)
        return UPF;
    else if(strcmp(type, "UPF_MEC") == 0)
        return UPF_MEC;
    else
        error("TrafficFlowFilter::selectOwnerType - unknown owner type [%s]. Aborting...",type);

    // never gets here
    return ENB;
}

void TrafficFlowFilter::handleMessage(cMessage *msg)
{
    EV << "TrafficFlowFilter::handleMessage - Received Packet:" << endl;
    EV << "name: " << msg->getFullName() << endl;

    Packet* pkt = check_and_cast<Packet *>(msg);

    // receive and read IP datagram
    // TODO: needs to be adapted for IPv6
    const auto& ipv4Header = pkt->peekAtFront<Ipv4Header>();
    const Ipv4Address &destAddr = ipv4Header->getDestAddress();
    const Ipv4Address &srcAddr = ipv4Header->getSrcAddress();
    pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);

    // TODO check for source and dest port number

    EV << "TrafficFlowFilter::handleMessage - Received datagram : " << pkt->getName() << " - src[" << srcAddr << "] - dest[" << destAddr << "]\n";

    // run packet filter and associate a flowId to the connection (default bearer?)
    // search within tftTable the proper entry for this destination
    TrafficFlowTemplateId tftId = findTrafficFlow(srcAddr, destAddr);   // search for the tftId in the binder

    // add control info to the normal ip datagram. This info will be read by the GTP-U application
    auto tftInfo = pkt->addTag<TftControlInfo>();
    tftInfo->setTft(tftId);

    EV << "TrafficFlowFilter::handleMessage - setting tft=" << tftId << endl;

    // send the datagram to the GTP-U module
    send(pkt,"gtpUserGateOut");
}

TrafficFlowTemplateId TrafficFlowFilter::findTrafficFlow(L3Address srcAddress, L3Address destAddress)
{
    // check whether the destination address is a (simulated) MEC host's address
    if (binder_->isMecHost(destAddress))
    {
        // check if the destination belongs to another core network (for multi-operator scenarios)
        const char* destGw = (inet::L3AddressResolver().findHostWithAddress(destAddress))->getAncestorPar("gateway").stringValue();
        if (strcmp(gateway_, destGw) != 0)
        {
            // the destination is a MEC host under a different core network, send the packet to the gateway
            return -1;
        }

        EV << "TrafficFlowFilter::findTrafficFlow - returning flowId (-3) for tunneling to " << destAddress.str() << endl;
        return -3;
    }
    // emulation mode
    else if (!meAppsExtAddress_.isUnspecified() && destAddress.matches(meAppsExtAddress_, meAppsExtAddressMask_))
    {
        // the destination is a MecApplication running outside the simulator, forward to meHost (it has forwarding enabled)
        EV << "TrafficFlowFilter::findTrafficFlow - returning flowId (-3) for tunneling to " << destAddress.str() << " (external) " << endl;
        return -3;
    }

    MacNodeId destId = binder_->getMacNodeId(destAddress.toIpv4());
    if (destId == 0)
    {
        EV << "TrafficFlowFilter::findTrafficFlow - destination "<< destAddress.str() << " is not a UE. ";
        if (ownerType_ == UPF || ownerType_ == PGW)
        {
            EV << "Remove packet from the simulation." << endl;
            return -2;   // the destination UE has been removed from the simulation
        }
        else // BS or MEC
        {
            EV << "Forward packet to the gateway." << endl;
            return -1;   // the destination might be outside the cellular network, send the packet to the gateway
        }
    }

    MacNodeId destBS = binder_->getNextHop(destId);
    if (destBS == 0)
    {
        EV << "TrafficFlowFilter::findTrafficFlow - destination " << destAddress.str() << " is a UE [" << destId << "] not attached to any BS. Remove packet from the simulation." << endl;
        return -2;   // the destination UE is not attached to any nodeB
    }

    // the serving node for the UE might be a secondary node in case of NR Dual Connectivity
    // obtains the master node, if any (the function returns destEnb if it is a master already)
    MacNodeId destMaster = binder_->getMasterNode(destBS);
    MacNodeId srcMaster = binder_->getNextHop(binder_->getMacNodeId(srcAddress.toIpv4()));

    if (isBaseStation(ownerType_))
    {
        if (fastForwarding_ && srcMaster == destMaster)
            return 0;                 // local delivery

        return -1;   // send the packet to the PGW/UPF. It will forward the packet to the correct BS
                     // TODO if the BS is within the same core network, there should be a direct tunnel to
                     //      it without going through the gateway (for now, this is not implemented as it
                     //      may cause packets being transmitted via the X2

    }

    // MEC host or PGW/UPF

    // check if the destination belongs to another core network (for multi-operator scenarios)
    const char* destGw = binder_->getModuleByMacNodeId(destMaster)->par("gateway");
    if (strcmp(gateway_, destGw) != 0)
    {
        // the destination is a Base Station under a different core network, send the packet to the gateway
        EV << "Forward packet to the gateway" << endl;
        return -1;
    }

    EV << "Forward packet to BS " << destMaster << endl;
    return destMaster;
}

