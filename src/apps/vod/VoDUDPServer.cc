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

Define_Module(VoDUDPServer);
using namespace std;
using namespace inet;

VoDUDPServer::VoDUDPServer()
{
}
VoDUDPServer::~VoDUDPServer()
{
}

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
    double one = 1.0;
    TIME_SLOT = one / fps;
    numStreams = 0;

    // set up Udp socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(serverPort);
    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    if (!inputFileName.empty())
    {
        // Check whether string is empty
        if (traceType.compare("SVC") != 0)
            throw cRuntimeError("VoDUDPServer::initialize - only SVC trace is currently available. Abort.");

        infile.open(inputFileName.c_str(), ios::in);
        if (infile.bad()) /* Or file is bad */
            throw cRuntimeError("Error while opening input file (File not found or incorrect type)");

        infile.seekg(0, ios::beg);
        long int i = 0;
        while (!infile.eof())
        {
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

    /* Initialize parameters after the initialize() method */
    EV << "VoD Server initialize: Trace: " << inputFileName << " trace type " << traceType << endl;
    cMessage* timer = new cMessage("Timer");
    double start = par("startTime");
    double offset = (double) start + simTime().dbl();
    scheduleAt(offset, timer);
}

void VoDUDPServer::finish()
{
    if (infile.is_open())
        infile.close();
}

void VoDUDPServer::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "Timer"))
        {
            clientsPort = par("destPort");
            //     vclientsPort = cStringTokenizer(clientsPort).asIntVector();

            clientsStartStreamTime = par("clientsStartStreamTime").doubleValue();
            //vclientsStartStreamTime = cStringTokenizer(clientsStartStreamTime).asDoubleVector();

            clientsReqTime = par("clientsReqTime");
            vclientsReqTime = cStringTokenizer(clientsReqTime).asDoubleVector();

            int size = 0;

            const char *destAddrs = par("destAddresses");
            cStringTokenizer tokenizer(destAddrs);
            const char *token;
            while ((token = tokenizer.nextToken()) != nullptr)
            {
                clientAddr.push_back(L3AddressResolver().resolve(token));
                size++;
            }

            /* Register video streams*/

            for (int i = 0; i < size; i++)
            {
                M1Message* M1 = new M1Message();
                M1->setClientAddr(clientAddr[i]);
                M1->setClientPort(clientsPort);
                double npkt;
                npkt = clientsStartStreamTime;
                M1->setNumPkSent((int) (npkt * fps));

                numStreams++;
                EV << "VoD Server self message: Dest IP: " << clientAddr[i] << " port: " << clientsPort << " start stream: " << (int)(npkt* fps) << endl;
//                    scheduleAt(simTime() + vclientsReqTime[i], M1);
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
    M1Message* msgNew = (M1Message*) msg;
    long numPkSentApp = msgNew->getNumPkSent();
    if (svcTrace_[numPkSentApp].index == LONG_MAX)
    {
        /* End of file, send finish packet */
        Packet* fm = new Packet("VoDFinishPacket");
        socket.sendTo(fm, msgNew->getClientAddr(), msgNew->getClientPort());
        return;
    }
    else
    {
        int seq_num = numPkSentApp;
        int currentFrame = svcTrace_[numPkSentApp].frameNumber;

        Packet* packet = new Packet("VoDPacket");
        auto frame = makeShared<VoDPacket>();
        frame->setFrameSeqNum(seq_num);
        frame->setPayloadTimestamp(simTime());
        frame->setChunkLength(B(svcTrace_[numPkSentApp].length));
        frame->setFrameLength(svcTrace_[numPkSentApp].length + 2 * sizeof(int)); /* Seq_num plus frame length plus payload */
        frame->setTid(svcTrace_[numPkSentApp].tid);
        frame->setQid(svcTrace_[numPkSentApp].qid);
        frame->addTag<CreationTimeTag>()->setCreationTime(simTime());
        packet->insertAtBack(frame);

        socket.sendTo(packet, msgNew->getClientAddr(), msgNew->getClientPort());
        numPkSentApp++;
        while (1)
        {
            /* Get infos about the frame from file */

            if (svcTrace_[numPkSentApp].index == LONG_MAX)
                break;

            int seq_num = numPkSentApp;
            if (svcTrace_[numPkSentApp].frameNumber != currentFrame)
                break; // Finish sending packets belonging to the current frame

            Packet* packet = new Packet("VoDPacket");
            auto frame = makeShared<VoDPacket>();
            frame->setTid(svcTrace_[numPkSentApp].tid);
            frame->setQid(svcTrace_[numPkSentApp].qid);
            frame->setFrameSeqNum(seq_num);
            frame->setPayloadTimestamp(simTime());
            frame->setChunkLength(B(svcTrace_[numPkSentApp].length));
            frame->setFrameLength(svcTrace_[numPkSentApp].length + 2 * sizeof(int)); /* Seq_num plus frame length plus payload */
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

