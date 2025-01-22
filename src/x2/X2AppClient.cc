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

#include <omnetpp.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/sctp/SctpAssociation.h>
#include "x2/X2AppClient.h"
#include "common/binder/Binder.h"
#include <inet/transportlayer/contract/sctp/SctpCommand_m.h>
#include "stack/mac/LteMacEnb.h"

namespace simu5g {

Define_Module(X2AppClient);

using namespace omnetpp;
using namespace inet;

void X2AppClient::initialize(int stage)
{
    SctpClient::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        x2ManagerOut_ = gate("x2ManagerOut");
    }
    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        Binder *binder = inet::getModuleFromPar<Binder>(par("binderModule"), this);

        // TODO set the connect address
        // Automatic configuration not yet supported. Use the .ini file to set IP addresses

        // get the connectAddress and the corresponding X2 id
        L3Address addr = L3AddressResolver().resolve(par("connectAddress").stringValue());
        X2NodeId peerId = binder->getX2NodeId(addr.toIpv4());

        X2NodeId nodeId = check_and_cast<LteMacEnb *>(getContainingNode(this)->getSubmodule("cellularNic")->getSubmodule("mac"))->getMacCellId();
        binder->setX2PeerAddress(nodeId, peerId, addr);

        // set the connect port
        int connectPort = binder->getX2Port(peerId);
        par("connectPort").setIntValue(connectPort);
    }
}

void X2AppClient::socketEstablished(inet::SctpSocket *socket, unsigned long int buffer)
{
    EV << "X2AppClient: connected\n";
}

void X2AppClient::socketDataArrived(SctpSocket *, Packet *msg, bool)
{
    packetsRcvd++;

    EV << "X2AppClient::socketDataArrived - Client received packet Nr " << packetsRcvd << " from Sctp\n";
    emit(packetReceivedSignal, msg);
    bytesRcvd += msg->getByteLength();

    msg->removeTagIfPresent<SctpSendReq>();

    if (msg->getDataLength() > B(0)) {
        EV << "X2AppClient::socketDataArrived - Forwarding packet to the X2 manager" << endl;

        // forward to x2manager
        send(msg, x2ManagerOut_);
    }
    else {
        EV << "X2AppClient::socketDataArrived - No encapsulated message. Discarding." << endl;
        throw cRuntimeError("X2AppClient::socketDataArrived: No encapsulated message.");
    }
}

} //namespace

