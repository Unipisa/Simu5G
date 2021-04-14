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

Define_Module(LteRlcMux);


using namespace omnetpp;

/*
 * Upper Layer handler
 */

void LteRlcMux::rlc2mac(cPacket *pkt)
{
    EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port RLC_to_MAC\n";

    // Send message
    send(pkt,macSap_[OUT_GATE]);
}

    /*
     * Lower layer handler
     */

void LteRlcMux::mac2rlc(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    switch (lteInfo->getRlcType())
    {
        case TM:
        EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port TM_Sap$o\n";
        send(pkt,tmSap_[OUT_GATE]);
        break;
        case UM:
        EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port UM_Sap$o\n";
        send(pkt,umSap_[OUT_GATE]);
        break;
        case AM:
        EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port AM_Sap$o\n";
        send(pkt,amSap_[OUT_GATE]);
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
    macSap_[IN_GATE] = gate("MAC_to_RLC");
    macSap_[OUT_GATE] = gate("RLC_to_MAC");
    tmSap_[IN_GATE] = gate("TM_Sap$i");
    tmSap_[OUT_GATE] = gate("TM_Sap$o");
    umSap_[IN_GATE] = gate("UM_Sap$i");
    umSap_[OUT_GATE] = gate("UM_Sap$o");
    amSap_[IN_GATE] = gate("AM_Sap$i");
    amSap_[OUT_GATE] = gate("AM_Sap$o");
}

void LteRlcMux::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << "LteRlcMux : Received packet " << pkt->getName() <<
    " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();
    if (incoming == macSap_[IN_GATE])
    {
        mac2rlc(pkt);
    }
    else
    {
        rlc2mac(pkt);
    }
    return;
}

void LteRlcMux::finish()
{
    // TODO make-finish
}
