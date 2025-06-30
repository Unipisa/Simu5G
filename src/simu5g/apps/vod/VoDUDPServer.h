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

#ifndef _LTE_VODUDPSRV_H_
#define _LTE_VODUDPSRV_H_

#include <fstream>

#include <omnetpp.h>

#include <inet/transportlayer/contract/udp/UdpControlInfo.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include <platdep/sockets.h>
#include "apps/vod/VoDUDPStruct.h"
#include "apps/vod/VoDPacket_m.h"
#include "apps/vod/M1Message_m.h"

namespace simu5g {

using namespace omnetpp;

class VoDUDPServer : public cSimpleModule
{
  protected:
    inet::UdpSocket socket;
    /* Server parameters */

    int serverPort;
    std::ifstream infile;
    std::string inputFileName;
    int fps;
    std::string traceType;
    std::fstream outfile;
    double TIME_SLOT;

    int clientsPort;
    double clientsStartStreamTime;

    std::vector<std::string> vclientsIP;

    std::vector<int> vclientsPort;
    std::vector<double> vclientsStartStreamTime;
    std::vector<double> vclientsReqTime;
    std::vector<inet::L3Address> clientAddr;

    /* Statistics */

    unsigned int numStreams;  // number of video streams served
    unsigned long numPkSent;  // total number of packets sent

    struct tracerec
    {
        uint32_t trec_time;
        uint32_t trec_size;
    };

    struct svcPacket
    {
        int tid = -1;
        int lid = -1;
        int qid = -1;
        int length = -1;
        int frameNumber = -1;
        int timestamp = -1;
        int currentFrame = -1;
        std::string memoryAdd;
        std::string isDiscardable;
        std::string isTruncatable;
        std::string isControl;
        std::string frameType;
        long int index;

        svcPacket()  {

        }
    };
    unsigned int nrec_;

    tracerec *trace_ = nullptr;

    std::vector<svcPacket> svcTrace_;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void finish() override;
    void handleMessage(cMessage *msg) override;
    virtual void handleSVCMessage(cMessage *msg);
};

} //namespace

#endif

