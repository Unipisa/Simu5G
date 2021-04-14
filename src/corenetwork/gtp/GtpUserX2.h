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

#include <omnetpp.h>
#include <map>
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "corenetwork/gtp/GtpUserMsg_m.h"
#include "common/binder/Binder.h"
#include "x2/packet/LteX2Message.h"

/**
 * GtpUserX2 is used for building data tunnels between GTP peers over X2, for handover procedure.
 * GtpUserX2 can receive two kind of packets:
 * a) LteX2Message from the X2 Manager. Those packets encapsulate an IP datagram
 * b) GtpUserX2Msg from UDP-IP layers.
 *
 */
class GtpUserX2 : public omnetpp::cSimpleModule
{
    inet::UdpSocket socket_;
    int localPort_;

    // reference to the LTE Binder module
    Binder* binder_;

    // the GTP protocol Port
    unsigned int tunnelPeerPort_;

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage);
    virtual void handleMessage(inet::cMessage *msg);

    // receive an X2 Message from the X2 Manager, encapsulates it in a GTP-U packet than forwards it to the proper next hop
    void handleFromStack(inet::Packet * x2Msg);

    // receive a GTP-U packet from UDP, detunnel it and send it to the X2 Manager
    void handleFromUdp(inet::Packet * gtpMsg);
};

#endif
