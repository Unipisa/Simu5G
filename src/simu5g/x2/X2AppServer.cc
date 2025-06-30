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

#include "x2/X2AppServer.h"

#include <inet/common/ProtocolTag_m.h>
#include <inet/common/TimeTag_m.h>
#include <inet/common/packet/Message.h>
#include <inet/common/socket/SocketTag_m.h>
#include <inet/transportlayer/contract/sctp/SctpCommand_m.h>

namespace simu5g {

Define_Module(X2AppServer);

using namespace omnetpp;
using namespace inet;

void X2AppServer::initialize(int stage)
{
    SctpServer::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        x2ManagerIn_ = gate("x2ManagerIn");

        X2NodeId id = MacNodeId(inet::getContainingNode(this)->par("macCellId").intValue());

        // register listening port to the binder. It will be used by
        // the client side as connectPort
        int localPort = par("localPort");
        Binder *binder = inet::getModuleFromPar<Binder>(par("binderModule"), this);
        binder->registerX2Port(id, localPort);
    }
}

// generate SCTP header and send packet
void X2AppServer::generateAndSend(Packet *pkt)
{
    pkt->addTag<DispatchProtocolReq>()->setProtocol(&Protocol::sctp);
    auto cmd = pkt->addTagIfAbsent<SctpSendReq>();
    cmd->setSocketId(assocId);
    cmd->setSendUnordered(ordered ? COMPLETE_MESG_ORDERED : COMPLETE_MESG_UNORDERED);

    lastStream = (lastStream + 1) % outboundStreams;
    cmd->setSid(lastStream);
    cmd->setPrValue(par("prValue"));
    cmd->setPrMethod((int32_t)par("prMethod"));

    if (queueSize > 0 && numRequestsToSend > 0 && count < queueSize * 2)
        cmd->setLast(false);
    else
        cmd->setLast(true);
    pkt->setKind(SCTP_C_SEND);
    packetsSent++;
    bytesSent += pkt->getBitLength() / 8;

    EV << "X2AppServer::generateAndSend: sending X2 message via SCTP: " << pkt->getId() << std::endl;

    sendOrSchedule(pkt);
}

void X2AppServer::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming == x2ManagerIn_) {
        EV << "X2AppServer::handleMessage - Received message from x2 manager" << endl;
        EV << "X2AppServer::handleMessage - Forwarding to X2 interface" << endl;

        Packet *pkt = check_and_cast<Packet *>(msg);

        // generate an SCTP packet and send it to the lower layer
        generateAndSend(pkt);
    }
    else {
        SctpServer::handleMessage(msg);
    }
}

void X2AppServer::handleTimer(cMessage *msg)
{
    SctpServer::handleTimer(msg);
}

} //namespace

