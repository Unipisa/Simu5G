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

#include "apps/taskOffloading/TaskOffloadingRequester.h"

#include <cmath>
#include <inet/common/TimeTag_m.h>

#define round(x) floor((x) + 0.5)

Define_Module(TaskOffloadingRequester);
using namespace inet;
using namespace std;


simsignal_t TaskOffloadingRequester::taskOffloadingSentReq = registerSignal("taskOffloadingSentReq");
simsignal_t TaskOffloadingRequester::taskOffloadingRtt = registerSignal("taskOffloadingRtt");


TaskOffloadingRequester::TaskOffloadingRequester()
{
    initialized_ = false;
    selfSource_ = nullptr;
    selfSender_ = nullptr;
}

TaskOffloadingRequester::~TaskOffloadingRequester()
{
    cancelAndDelete(selfSource_);
}

void TaskOffloadingRequester::initialize(int stage)
{

    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        selfSource_ = new cMessage("selfSource");
        iDframe_ = 0;
        timestamp_ = 0;
        size_ = par("PacketSize");
        sampling_time = par("sampling_time");
        localPort_ = par("localPort");
        destPort_ = par("destPort");

        tempCounter_ = 0;
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        // calculating traffic starting time
        startTime_ = par("startTime");
        finishTime_ = par("finishTime");

        EV << " finish time " << finishTime_ << endl;

        initTraffic_ = new cMessage("initTraffic");
        initTraffic();
    }
}

void TaskOffloadingRequester::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSource"))
        {
            EV << "TaskOffloadingRequester::handleMessage - now[" << simTime() << "] <= finish[" << finishTime_ <<"]" <<endl;
            if( simTime() <= finishTime_ || finishTime_ == 0 )
                sendRequest();
        }
        else
            initTraffic();
    }
    else // receiving response
    {
        handleResponse(msg);
    }
}

void TaskOffloadingRequester::initTraffic()
{
    std::string destAddress = par("destAddress").stringValue();
    cModule* destModule = findModuleByPath(par("destAddress").stringValue());
    if (destModule == nullptr)
    {
        // this might happen when users are created dynamically
        EV << simTime() << "TaskOffloadingRequester::initTraffic - destination " << destAddress << " not found" << endl;

        simtime_t offset = 0.01; // TODO check value
        scheduleAt(simTime()+offset, initTraffic_);
        EV << simTime() << "TaskOffloadingRequester::initTraffic - the node will retry to initialize traffic in " << offset << " seconds " << endl;
    }
    else
    {
        delete initTraffic_;
        destAddress_ = inet::L3AddressResolver().resolve(par("destAddress").stringValue());

        // set primary and secondary address
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort_);

        int tos = par("tos");
        if (tos != -1)
            socket.setTos(tos);

        EV << simTime() << "TaskOffloadingRequester::initialize - binding to port: local:" << localPort_ << " , dest: " << destAddress_.str() << ":" << destPort_ << endl;

        // calculating traffic starting time
        simtime_t startTime = par("startTime");

        scheduleAt(simTime()+startTime, selfSource_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void TaskOffloadingRequester::sendRequest()
{
    Packet* packet = new Packet("OffloadRequest");
    auto req = makeShared<TaskOffloadingRequest>();
    req->setId(iDframe_++);
    req->setPayloadTimestamp(simTime());
    req->setPayloadSize(size_);
    req->setChunkLength(B(size_));
    req->setNumTaskInstructions(par("numTaskInstructions"));
    req->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(req);

    emit(taskOffloadingSentReq,iDframe_);

    EV << "TaskOffloadingRequester::sendCbrRequest - send service request" << endl;
    socket.sendTo(packet, destAddress_, destPort_);

    scheduleAt(simTime() + sampling_time, selfSource_);
}


void TaskOffloadingRequester::handleResponse(cMessage *msg)
{
    Packet* pPacket = check_and_cast<Packet*>(msg);
    auto toffHeader = pPacket->popAtFront<TaskOffloadingResponse>();

    simtime_t rtt = simTime()-toffHeader->getPayloadTimestamp();
    emit(taskOffloadingRtt,rtt );

    EV << "TaskOffloadingRequester::handleMessage - response received after " << rtt << " seconds." << endl;

    delete msg;
}

void TaskOffloadingRequester::finish()
{
}

