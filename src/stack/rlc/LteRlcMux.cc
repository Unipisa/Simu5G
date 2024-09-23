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

#include "stack/rlc/LteRlcMux.h"

namespace simu5g {

Define_Module(LteRlcMux);

using namespace omnetpp;

/*
 * Upper Layer handler
 */

void LteRlcMux::rlc2mac(cPacket *pkt)
{
    EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port RLC_to_MAC\n";

    // Send message
    send(pkt, macSapOutGate_);
}

/*
 * Lower Layer handler
 */

void LteRlcMux::mac2rlc(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    switch (lteInfo->getRlcType()) {
        case TM:
            EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port TM_Sap$o\n";
            send(pkt, tmSapOutGate_);
            break;
        case UM:
            EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port UM_Sap$o\n";
            send(pkt, umSapOutGate_);
            break;
        case AM:
            EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port AM_Sap$o\n";
            send(pkt, amSapOutGate_);
            break;
        default:
            throw cRuntimeError("LteRlcMux: wrong traffic type %d", lteInfo->getRlcType());
    }
}

/*
 * Main functions
 */

void LteRlcMux::initialize()
{
    macSapInGate_ = gate("MAC_to_RLC");
    macSapOutGate_ = gate("RLC_to_MAC");
    tmSapInGate_ = gate("TM_Sap$i");
    tmSapOutGate_ = gate("TM_Sap$o");
    umSapInGate_ = gate("UM_Sap$i");
    umSapOutGate_ = gate("UM_Sap$o");
    amSapInGate_ = gate("AM_Sap$i");
    amSapOutGate_ = gate("AM_Sap$o");
}

void LteRlcMux::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    EV << "LteRlcMux : Received packet " << pkt->getName() <<
        " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate *incoming = pkt->getArrivalGate();
    if (incoming == macSapInGate_) {
        mac2rlc(pkt);
    }
    else {
        rlc2mac(pkt);
    }
}

void LteRlcMux::finish()
{
    // TODO make-finish
}

} //namespace

