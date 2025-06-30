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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <inet/common/TimeTag_m.h>
#include "apps/vod/VoDUDPServer.h"

namespace simu5g {

Define_Module(VoDUDPServer);
using namespace std;
using namespace inet;



void VoDUDPServer::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage != INITSTAGE_APPLICATION_LAYER)
        return;
    EV << "VoD Server initialize: stage " << stage << endl;
    serverPort = par("localPort");
    inputFileName = par("vod_trace_file").stringValue();
    traceType = par("traceType").stringValue();
    fps = par("fps");
    TIME_SLOT = 1.0 / fps;
    numStreams = 0;

    // set up Udp socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(serverPort);
    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    if (!inputFileName.empty()) {
        // Check whether string is empty
        if (traceType != "SVC")
            throw cRuntimeError("VoDUDPServer::initialize - only SVC trace is currently available. Abort.");

        infile.open(inputFileName.c_str(), ios::in);
        if (infile.bad()) // Or the file is bad
            throw cRuntimeError("Error while opening input file (File not found or incorrect type)");

        infile.seekg(0, ios::beg);
        long int i = 0;
        while (!infile.eof()) {
            svcPacket tmp;
            tmp.index = i;
            infile >> tmp.memoryAdd >> tmp.length >> tmp.lid >> tmp.tid >> tmp.qid >>
            tmp.frameType >> tmp.isDiscardable >> tmp.isTruncatable >> tmp.frameNumber
            >> tmp.timestamp >> tmp.isControl;
            svcTrace_.push_back(tmp);
            i++;
        }
        svcPacket tmp;
        tmp.index = LONG_MAX;
        svcTrace_.push_back(tmp);
    }

    // Initialize parameters after the initialize() method
    EV << "VoD Server initialize: Trace: " << inputFileName << " trace type " << traceType << endl;
    cMessage *timer = new cMessage("Timer");
    double start = par("startTime");
    double offset = start + simTime().dbl();
    scheduleAt(offset, timer);
}

void VoDUDPServer::finish()
{
    if (infile.is_open())
        infile.close();
}

void VoDUDPServer::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (!strcmp(msg->getName(), "Timer")) {
            clientsPort = par("destPort");
            clientsStartStreamTime = par("clientsStartStreamTime").doubleValue();
            //vclientsStartStreamTime = cStringTokenizer(clientsStartStreamTime).asDoubleVector();

            vclientsReqTime = check_and_cast<cValueArray *>(par("clientsReqTime").objectValue())->asDoubleVectorInUnit("s");

            auto destAddrs = check_and_cast<cValueArray *>(par("clientsReqTime").objectValue())->asStringVector();
            for (const auto& token : destAddrs) {
                clientAddr.push_back(L3AddressResolver().resolve(token.c_str()));
            }

            // Register video streams

            for (const auto & i : clientAddr) {
                M1Message *M1 = new M1Message();
                M1->setClientAddr(i);
                M1->setClientPort(clientsPort);
                double npkt = clientsStartStreamTime;
                M1->setNumPkSent((int)(npkt * fps));

                numStreams++;
                EV << "VoD Server self message: Dest IP: " << i << " port: " << clientsPort << " start stream: " << (int)(npkt * fps) << endl;
                scheduleAt(simTime(), M1);
            }

            delete msg;
            return;
        }
        else
            handleSVCMessage(msg);
    }
    else
        delete msg;
}

void VoDUDPServer::handleSVCMessage(cMessage *msg)
{
    M1Message *msgNew = static_cast<M1Message *>(msg);
    long numPkSentApp = msgNew->getNumPkSent();
    if (svcTrace_[numPkSentApp].index == LONG_MAX) {
        // End of file, send finish packet
        Packet *fm = new Packet("VoDFinishPacket");
        socket.sendTo(fm, msgNew->getClientAddr(), msgNew->getClientPort());
        return;
    }
    else {
        int seq_num = numPkSentApp;
        int currentFrame = svcTrace_[numPkSentApp].frameNumber;

        Packet *packet = new Packet("VoDPacket");
        auto frame = makeShared<VoDPacket>();
        frame->setFrameSeqNum(seq_num);
        frame->setPayloadTimestamp(simTime());
        frame->setChunkLength(B(svcTrace_[numPkSentApp].length));
        frame->setFrameLength(svcTrace_[numPkSentApp].length + 2 * sizeof(int)); // Seq_num plus frame length plus payload
        frame->setTid(svcTrace_[numPkSentApp].tid);
        frame->setQid(svcTrace_[numPkSentApp].qid);
        frame->addTag<CreationTimeTag>()->setCreationTime(simTime());
        packet->insertAtBack(frame);

        socket.sendTo(packet, msgNew->getClientAddr(), msgNew->getClientPort());
        numPkSentApp++;
        while (true) {
            // Get info about the frame from file

            if (svcTrace_[numPkSentApp].index == LONG_MAX)
                break;

            int seq_num = numPkSentApp;
            if (svcTrace_[numPkSentApp].frameNumber != currentFrame)
                break; // Finish sending packets belonging to the current frame

            Packet *packet = new Packet("VoDPacket");
            auto frame = makeShared<VoDPacket>();
            frame->setTid(svcTrace_[numPkSentApp].tid);
            frame->setQid(svcTrace_[numPkSentApp].qid);
            frame->setFrameSeqNum(seq_num);
            frame->setPayloadTimestamp(simTime());
            frame->setChunkLength(B(svcTrace_[numPkSentApp].length));
            frame->setFrameLength(svcTrace_[numPkSentApp].length + 2 * sizeof(int)); // Seq_num plus frame length plus payload
            frame->addTag<CreationTimeTag>()->setCreationTime(simTime());
            packet->insertAtBack(frame);
            socket.sendTo(packet, msgNew->getClientAddr(), msgNew->getClientPort());
            EV << " VoDUDPServer::handleSVCMessage sending frame " << seq_num << std::endl;
            numPkSentApp++;
        }
        msgNew->setNumPkSent(numPkSentApp);
        scheduleAt(simTime() + TIME_SLOT, msgNew);
    }
}

} //namespace

