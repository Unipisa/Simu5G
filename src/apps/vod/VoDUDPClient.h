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

namespace simu5g {

using namespace omnetpp;

class VoDUDPClient : public cSimpleModule
{
    inet::UdpSocket socket;
    std::fstream outfile;
    unsigned int totalRcvdBytes_;

  public:
    simsignal_t tptLayer0_;
    simsignal_t tptLayer1_;
    simsignal_t tptLayer2_;
    simsignal_t tptLayer3_;
    simsignal_t delayLayer0_;
    simsignal_t delayLayer1_;
    simsignal_t delayLayer2_;
    simsignal_t delayLayer3_;

  protected:

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void finish() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void receiveStream(const VoDPacket *msg);
};

} //namespace

#endif

