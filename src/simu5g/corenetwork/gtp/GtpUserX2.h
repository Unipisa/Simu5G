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

#ifndef __GTP_USER_X2_H_
#define __GTP_USER_X2_H_

#include <map>

#include <inet/common/ModuleRefByPar.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "common/binder/Binder.h"
#include "corenetwork/gtp/GtpUserMsg_m.h"
#include "x2/packet/LteX2Message.h"

namespace simu5g {

using namespace omnetpp;

/**
 * GtpUserX2 is used for building data tunnels between GTP peers over X2, for handover procedures.
 * GtpUserX2 can receive two kinds of packets:
 * a) LteX2Message from the X2 Manager. These packets encapsulate an IP datagram
 * b) GtpUserX2Msg from UDP-IP layers.
 *
 */
class GtpUserX2 : public cSimpleModule
{
    inet::UdpSocket socket_;
    int localPort_;

    // reference to the LTE Binder module
    inet::ModuleRefByPar<Binder> binder_;

    // the GTP protocol Port
    unsigned int tunnelPeerPort_;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(inet::cMessage *msg) override;

    // receive an X2 Message from the X2 Manager, encapsulates it in a GTP-U packet then forwards it to the proper next hop
    void handleFromStack(inet::Packet *x2Msg);

    // receive a GTP-U packet from UDP, detunnel it and send it to the X2 Manager
    void handleFromUdp(inet::Packet *gtpMsg);
};

} //namespace

#endif

