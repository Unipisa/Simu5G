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

#include "apps/vod/VoDUDPClient.h"
#include <fcntl.h>

namespace simu5g {

using namespace std;
using namespace inet;

Define_Module(VoDUDPClient);

simsignal_t VoDUDPClient::tptLayer0Signal_ = registerSignal("VoDTptLayer0");
simsignal_t VoDUDPClient::tptLayer1Signal_ = registerSignal("VoDTptLayer1");
simsignal_t VoDUDPClient::tptLayer2Signal_ = registerSignal("VoDTptLayer2");
simsignal_t VoDUDPClient::tptLayer3Signal_ = registerSignal("VoDTptLayer3");

simsignal_t VoDUDPClient::delayLayer0Signal_ = registerSignal("VoDDelayLayer0");
simsignal_t VoDUDPClient::delayLayer1Signal_ = registerSignal("VoDDelayLayer1");
simsignal_t VoDUDPClient::delayLayer2Signal_ = registerSignal("VoDDelayLayer2");
simsignal_t VoDUDPClient::delayLayer3Signal_ = registerSignal("VoDDelayLayer3");

void VoDUDPClient::initialize(int stage)
{
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    // Get parameters from INI file
    EV << "VoD Client initialized: stage " << stage << endl;

    stringstream ss;
    ss << getId();
    string dir = "./Framework/clients/client" + ss.str() + "/";
    string trace = "./Framework/clients/client" + ss.str() + "/trace/";

#if defined(_WIN32)
    _mkdir(dir.c_str());
    _mkdir(trace.c_str());
#else
    mode_t dir_permissions = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH;
    mkdir(dir.c_str(), dir_permissions);
    mkdir(trace.c_str(), dir_permissions);
#endif

    string nsOutput = "./Framework/clients/client" + ss.str() + "/nsout.txt";
    outfile.open(nsOutput.c_str(), ios::out);

    if (outfile.bad()) // File is bad
        throw cRuntimeError("Error while opening output file (File not found or incorrect type)");

    totalRcvdBytes_ = 0;

    cMessage *timer = new cMessage("Timer");
    scheduleAt(simTime(), timer);
}

void VoDUDPClient::finish()
{
    outfile.close();
    bool startMetrics = par("startMetrics");

    // Parameters to be sent to ana.sh

    string inputFileName = par("vod_trace_file").stringValue();
    string bsePath = par("bsePath").stringValue();
    string origVideoYuv = par("origVideoYuv").stringValue();
    string origVideoSvc = par("origVideoSvc").stringValue();
    string decPath = par("decPath").stringValue();
    string avipluginPath = par("avipluginPath").stringValue();
    int playbackSize = par("playbackSize");
    string traceType = par("traceType");
    int numPktPerFrame = par("numPktPerFrame");
    int numFrame = par("numFrame");

    if (startMetrics) {
        stringstream ss, nf, pb, npktf;
        ss << getId();
        pb << playbackSize;
        npktf << numPktPerFrame;
        nf << numFrame;

        string createOutput = "./Framework/creatensoutput.py " + inputFileName + " " + ss.str();

        string anaPar = "./Framework/ana.sh " + inputFileName + " " + bsePath + " " + origVideoYuv
            + " " + origVideoSvc + " " + decPath + " " + pb.str()
            + " " + avipluginPath + " " + ss.str() + " " + nf.str();

        // Bin FPS Configurable NumPkt frame
        string plot = "./Framework/plot.py 25 ./Framework/clients/client" + ss.str()
            + "/output/nsoutput.txt-plos ./Framework/clients/client" + ss.str()
            + "/output/nsoutput.txt-psnr 25 " + nf.str() + " " + npktf.str() + " " + ss.str();

        fstream f;
        string apri = "client" + ss.str() + ".sh";
        f.open(apri.c_str(), ios::out);
        f << "#!/bin/bash" << endl;
        f << createOutput << " && " << anaPar << " && " << plot << endl;
        f.close();
        //system(createOutput.c_str());
        //system(anaPar.c_str());
        //system(plot.c_str());
    }
    else {
        if (traceType == "ns2") {
            stringstream ss, pb, np;
            ss << getId();
            pb << playbackSize;
            double startStreamTime = par("startStreamTime");
            int npkt = (int)startStreamTime;
            np << npkt;

            string args = inputFileName + " " + pb.str() + " " + ss.str() + " " + np.str();
            FILE *fp = popen("./Framework/createns2output.py", "w");
            if (fp != nullptr) {
                fprintf(fp, "%s", args.c_str());
                fclose(fp);
            }
        }
    }
}

void VoDUDPClient::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        int localPort = par("localPort");
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort);
        int tos = par("tos");
        if (tos != -1)
            socket.setTos(tos);
        delete msg;
    }
    else if (!strcmp(msg->getName(), "VoDPacket")) {
        receiveStream(check_and_cast<inet::Packet *>(msg)->peekAtFront<VoDPacket>().get());
        delete msg;
    }
    else
        delete msg;
}

void VoDUDPClient::receiveStream(const VoDPacket *msg)
{
    // int seqNum = msg->getFrameSeqNum();
    simtime_t sendingTime = msg->getPayloadTimestamp();
    // int frameLength = msg->getFrameLength();
    simtime_t delay = simTime() - sendingTime;
    int layer = msg->getQid();

    totalRcvdBytes_ += msg->getFrameLength();
    double tputSample = (double)totalRcvdBytes_ / (simTime() - getSimulation()->getWarmupPeriod());
    if (layer == 0) {
        emit(tptLayer0Signal_, tputSample);
        emit(delayLayer0Signal_, delay.dbl());
    }
    else if (layer == 1) {
        emit(tptLayer1Signal_, tputSample);
        emit(delayLayer1Signal_, delay.dbl());
    }
    else if (layer == 2) {
        emit(tptLayer2Signal_, tputSample);
        emit(delayLayer2Signal_, delay.dbl());
    }
    else if (layer == 3) {
        emit(tptLayer3Signal_, tputSample);
        emit(delayLayer3Signal_, delay.dbl());
    }
}

} //namespace

