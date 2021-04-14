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
//

#ifndef _LTE_VODUDPCLIENT_H_
#define _LTE_VODUDPCLIENT_H_

#include <omnetpp.h>
#include <string.h>
#include <fstream>

#include <apps/vod/VoDPacket_m.h>
#include <apps/vod/VoDUDPStruct.h>
#include <inet/transportlayer/contract/udp/UdpControlInfo.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

class VoDUDPClient : public omnetpp::cSimpleModule
{
    inet::UdpSocket socket;
    std::fstream outfile;
    unsigned int totalRcvdBytes_;

  public:
    omnetpp::simsignal_t tptLayer0_;
    omnetpp::simsignal_t tptLayer1_;
    omnetpp::simsignal_t tptLayer2_;
    omnetpp::simsignal_t tptLayer3_;
    omnetpp::simsignal_t delayLayer0_;
    omnetpp::simsignal_t delayLayer1_;
    omnetpp::simsignal_t delayLayer2_;
    omnetpp::simsignal_t delayLayer3_;

  protected:

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void finish() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void receiveStream(VoDPacket *msg);
};

#endif

