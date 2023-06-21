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

#include "apps/taskOffloading/TaskOffloadingResponder.h"

Define_Module(TaskOffloadingResponder);
using namespace inet;

simsignal_t TaskOffloadingResponder::taskOffloadingRcvdReq = registerSignal("taskOffloadingRcvdReq");

void TaskOffloadingResponder::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        mInit_ = true;
        numReceived_ = 0;
        recvBytes_ = 0;
        respSize_ = par("responseSize");
        enableVimComputing_ = par("enableVimComputing").boolValue();
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        int port = par("localPort");
        EV << "TaskOffloadingResponder::initialize - binding to port: local:" << port << endl;
        if (port != -1)
        {
            socket.setOutputGate(gate("socketOut"));
            socket.bind(port);

            destAddress_ = inet::L3AddressResolver().resolve(par("destAddress").stringValue());
            destPort_ = par("destPort");

            if(enableVimComputing_)
            {
                vim_ = check_and_cast<VirtualisationInfrastructureManager*>(getParentModule()->getSubmodule("vim"));
                appId_ = getId();
                bool res = vim_->registerMecApp(appId_, par("requiredRam").doubleValue(), par("requiredDisk").doubleValue(), par("requiredCpu").doubleValue());
                if(res == false)
                {
                    throw cRuntimeError("TaskOffloadingResponder::initialize - registration to the VIM failed");
                }
            }

            processingTimer_  = new cMessage("computeMsg");
        }
    }
}

void TaskOffloadingResponder::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        socket.sendTo(respPacket_, destAddress_, destPort_);
        return;
    }
    else
    {
        Packet* pPacket = check_and_cast<Packet*>(msg);
        auto toffHeader = pPacket->popAtFront<TaskOffloadingRequest>();

        numReceived_++;
        int numTaskInstructions = toffHeader->getNumTaskInstructions();

        EV << "TaskOffloadingResponder::handleMessage - Packet received: ID[" << toffHeader->getId() << "] - number of instructions to execute[" << numTaskInstructions << "]" << endl;

        emit(taskOffloadingRcvdReq, (long)toffHeader->getId());


        // generate reply
        simtime_t processingTime = 0;
        if( enableVimComputing_ )
        {
            processingTime = vim_->calculateProcessingTime(appId_, numTaskInstructions) ;
            EV << "TaskOffloadingResponder::handleMessage - requesting an Edge-Based processing time of " << processingTime << " seconds for " << numTaskInstructions << " instructions" <<endl;
//            std::cout << "TaskOffloadingResponder::handleMessage - requesting an Edge-Based processing time of " << processingTime << " seconds for " << numTaskInstructions << " instructions" <<endl;

        }

        respPacket_ = new Packet("OffloadResult");
        auto resp = makeShared<TaskOffloadingResponse>();
        resp->setId((int)toffHeader->getId());
        resp->setPayloadTimestamp(toffHeader->getPayloadTimestamp());
        resp->setPayloadSize(respSize_);
        resp->setChunkLength(B(respSize_));
        //cbr->addTag<CreationTimeTag>()->setCreationTime(simTime());
        respPacket_->insertAtBack(resp);

        EV << "TaskOffloadingResponder::handleMessage - scheduling response in " << processingTime << " seconds." << endl;
        scheduleAfter(processingTime, processingTimer_);

        delete msg;
    }
}

void TaskOffloadingResponder::finish()
{
}



