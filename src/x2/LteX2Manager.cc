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


#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/networklayer/configurator/ipv4/Ipv4NetworkConfigurator.h>
#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>
#include "x2/LteX2Manager.h"

namespace simu5g {

Define_Module(LteX2Manager);

using namespace omnetpp;
using namespace inet;

constexpr const char *DATAPORT_OUT = "dataPort$o";

void LteX2Manager::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        // get the node id
        nodeId_ = MacNodeId(inet::getContainingNode(this)->par("macCellId").intValue());

        // get reference to the binder
        binder_.reference(this, "binderModule", true);
    }
    else if (stage == inet::INITSTAGE_NETWORK_LAYER) {
        // find x2ppp interface entries and register their IP addresses to the binder
        // IP addresses will be used in the next init stage to get the X2 id of the peer
        IInterfaceTable *interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
            // look for x2ppp interfaces in the interface table
            NetworkInterface *NetworkInterface = interfaceTable->getInterface(i);
            const char *ifName = NetworkInterface->getInterfaceName();
            if (strstr(ifName, "x2ppp") != nullptr) {
                const Ipv4Address addr = NetworkInterface->getProtocolData<Ipv4InterfaceData>()->getIPAddress();
                binder_->setX2NodeId(addr, nodeId_);
            }
        }
    }
    else if (stage == inet::INITSTAGE_TRANSPORT_LAYER) {
        // for each X2App, get the client submodule and set connection parameters (connectPort)
        for (int i = 0; i < gateSize("x2$i"); i++) {
            // client of the X2Apps is connected to the input sides of "x2" gate
            cGate *inGate = gate("x2$i", i);

            // get the X2App client connected to this gate
            //                                                  x2  -> X2App.x2ManagerIn ->  X2App.client
            X2AppClient *client = check_and_cast<X2AppClient *>(inGate->getPathStartGate()->getOwnerModule());

            // get the connectAddress for the X2App client and the corresponding X2 id
            L3Address addr = L3AddressResolver().resolve(client->par("connectAddress").stringValue());
            X2NodeId peerId = binder_->getX2NodeId(addr.toIpv4());

            // bind the peerId to the output gate
            x2InterfaceTable_[peerId] = i;
        }
    }
}

void LteX2Manager::handleMessage(cMessage *msg)
{
    Packet *pkt = check_and_cast<Packet *>(msg);
    cGate *incoming = pkt->getArrivalGate();

    // the incoming gate is part of a gate vector, so get the base name
    if (strcmp(incoming->getBaseName(), "dataPort") == 0) {
        // incoming data from LTE stack
        EV << "LteX2Manager::handleMessage - Received message from LTE stack" << endl;
        fromStack(pkt);
    }
    else { // from X2 or X2GTP
        if (strcmp(incoming->getBaseName(), "x2Gtp") == 0) {
            // incoming data from X2GTP
            EV << "LteX2Manager::handleMessage - Received message from X2-GTP" << endl;
        }
        else { // from X2
            if (strcmp(incoming->getBaseName(), "x2") != 0) {
                throw cRuntimeError(this, "LteX2Manager::handleMessage: invalid incoming gate");
            }

            // the incoming gate belongs to a gate vector, so get its index
            int gateIndex = incoming->getIndex();

            // incoming data from X2
            EV << "LteX2Manager::handleMessage - Received message from X2, gate " << gateIndex << endl;
        }
        // call handler
        fromX2(pkt);
    }
}

void LteX2Manager::fromStack(Packet *pkt)
{
    auto x2msg = pkt->removeAtFront<LteX2Message>();
    auto x2Info = pkt->removeTagIfPresent<X2ControlInfoTag>();
    if (x2Info->getInit()) {
        // gate initialization
        LteX2MessageType msgType = x2msg->getType();
        int gateIndex = pkt->getArrivalGate()->getIndex();
        dataInterfaceTable_[msgType] = gateIndex;

        delete pkt;
        return;
    }

    // If the message is carrying data, send it to the GTPUserX2 module
    // (GTPUserX2 module will tunnel this datagram towards the target eNB)
    // otherwise it is an X2 control message and is sent to the x2 peer
    DestinationIdList destList = x2Info->getDestIdList();
    for (auto targetEnb : destList) {
        auto pktDuplicate = pkt->dup();
        x2msg->markMutableIfExclusivelyOwned();
        x2msg->setSourceId(nodeId_);
        x2msg->setDestinationId(targetEnb);
        pktDuplicate->insertAtFront(x2msg);

        cGate *outputGate;
        if (x2msg->getType() == X2_HANDOVER_DATA_MSG || x2msg->getType() == X2_DUALCONNECTIVITY_DATA_MSG) {
            // send to the gate connected to the GTPUser module
            outputGate = gate("x2Gtp$o");
        }
        else {
            // select the index for the output gate (it belongs to a vector)
            int gateIndex = x2InterfaceTable_[targetEnb];
            outputGate = gate("x2$o", gateIndex);
        }

        send(pktDuplicate, outputGate);
    }
    delete pkt;
}

void LteX2Manager::fromX2(Packet *pkt)
{
    auto x2msg = pkt->peekAtFront<LteX2Message>();
    LteX2MessageType msgType = x2msg->getType();

    if (msgType == X2_UNKNOWN_MSG) {
        EV << "LteX2Manager::fromX2 - Unknown type of the X2 message. Discard." << endl;
        return;
    }

    // get the correct output gate for the message
    int gateIndex = dataInterfaceTable_[msgType];
    cGate *outGate = gate(DATAPORT_OUT, gateIndex);

    // send X2 msg to stack
    EV << "LteX2Manager::fromX2 - send X2MSG to LTE stack" << endl;
    send(pkt, outGate);
}

} //namespace

