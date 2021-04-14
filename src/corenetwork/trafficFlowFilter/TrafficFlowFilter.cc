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

    //mec
    if(getParentModule()->hasPar("meHost")){

        meHost = getParentModule()->par("meHost").stdstringValue();
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
    registerService(LteProtocol::ipv4uu, gate("internetFilterGateIn"), gate("internetFilterGateIn"));
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
    else if(strcmp(type, "GTPENDPOINT") == 0)
        return GTPENDPOINT;
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
    if(tftId == -2)
    {
        // the destination has been removed from the simulation. Delete msg
        EV << "TrafficFlowFilter::handleMessage - Destination has lost its connection or has been removed from the simulation. Delete packet." << endl;
        delete msg;
    }
    else
    {

        // add control info to the normal ip datagram. This info will be read by the GTP-U application
        auto tftInfo = pkt->addTag<TftControlInfo>();
        tftInfo->setTft(tftId);

        EV << "TrafficFlowFilter::handleMessage - setting tft=" << tftId << endl;

        // send the datagram to the GTP-U module
        send(pkt,"gtpUserGateOut");
    }
}

TrafficFlowTemplateId TrafficFlowFilter::findTrafficFlow(L3Address srcAddress, L3Address destAddress)
{
    //mec
    // check this before the other!
    if (isBaseStation(ownerType_) && destAddress.operator == (meHostAddress))
    {
        // the destination is the ME Host
        EV << "TrafficFlowFilter::findTrafficFlow - returning flowId (-3) for tunneling to " << meHost << endl;
        return -3;
    }
    else if(ownerType_ == GTPENDPOINT)
    {
        // send only messages direct to UEs --> UEs have macNodeId != 0
        MacNodeId destId = binder_->getMacNodeId(destAddress.toIpv4());
        MacNodeId destMaster = binder_->getNextHop(destId);
        EV << "TrafficFlowFilter::findTrafficFlow - returning flowId for " <<  binder_->getModuleNameByMacNodeId(destMaster) <<": "<< destMaster << endl;
        return destMaster;
    }
    //end mec
    //

    MacNodeId destId = binder_->getMacNodeId(destAddress.toIpv4());
    if (destId == 0)
    {
        EV << "TrafficFlowFilter::findTrafficFlow - destId = "<< destId << endl;

        if (isBaseStation(ownerType_))
            return -1;   // the destination is outside the LTE network, so send the packet to the PGW
        else // PGW or UPF
            return -2;   // the destination UE has been removed from the simulation
    }

    MacNodeId destBS = binder_->getNextHop(destId);
    if (destBS == 0)
        return -2;   // the destination UE is not attached to any nodeB

    // the serving node for the UE might be a secondary node in case of NR Dual Connectivity
    // obtains the master node, if any (the function returns destEnb if it is a master already)
    MacNodeId destMaster = binder_->getMasterNode(destBS);

    if (isBaseStation(ownerType_))
    {
        MacNodeId srcMaster = binder_->getNextHop(binder_->getMacNodeId(srcAddress.toIpv4()));
        if (fastForwarding_ && srcMaster == destMaster)
            return 0;                 // local delivery
        return -1;   // send the packet to the PGW/UPF
    }

    return destMaster;
}

