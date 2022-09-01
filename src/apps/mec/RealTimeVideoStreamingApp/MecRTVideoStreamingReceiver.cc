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

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include "apps/mec/RealTimeVideoStreamingApp/MecRTVideoStreamingReceiver.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_Types.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/httpUtils/json.hpp"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"

#include <fstream>

Define_Module(MecRTVideoStreamingReceiver);

using namespace inet;
using namespace omnetpp;

MecRTVideoStreamingReceiver::MecRTVideoStreamingReceiver(): MecAppBase()
{
    currentSessionId_ = -1;
    ueAppModule_ = nullptr;
}
MecRTVideoStreamingReceiver::~MecRTVideoStreamingReceiver()
{
    cancelAndDelete(displayFrame);
}


void MecRTVideoStreamingReceiver::initialize(int stage)
{
    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieving parameters
    size_ = par("packetSize");

    // set Udp Socket
    ueSocket.setOutputGate(gate("socketOut"));

    localUePort = par("localUePort");
    ueSocket.bind(localUePort);

    //testing
    EV << "MecRTVideoStreamingReceiver::initialize - Mec application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

    dropPackets_ = true;
    stopped = true;
    fps = 0 ;

    firstFrameDisplayed = false;
    lastFrameDisplayed_ = -1;
    expectedFrameDisplayed_ = 0;

    displayFrame = new cMessage("displayFrame");
}

void MecRTVideoStreamingReceiver::finish(){
    MecAppBase::finish();
    EV << "MecRTVideoStreamingReceiver::finish()" << endl;
}

void MecRTVideoStreamingReceiver::handleMessage(cMessage *msg)
{
    if (!msg->isSelfMessage())
    {
        if(ueSocket.belongsToSocket(msg))
        {
            handleUeMessage(msg);
            return;
        }
    }
    MecAppBase::handleMessage(msg);

}

void MecRTVideoStreamingReceiver::handleUeMessage(omnetpp::cMessage *msg)
{
    // determine its source address/port
    auto pk = check_and_cast<Packet *>(msg);
    ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

    // register statistics: they will be recorded at the UE side
    if (ueAppModule_ == nullptr)
    {
        ueAppModule_ = L3AddressResolver().findHostWithAddress(ueAppAddress)->getSubmodule("app",1);

        e2eDelaySegment_ = ueAppModule_->registerSignal("rtVideoStreamingEnd2endDelaySegment");
        segmentSize_ = ueAppModule_->registerSignal("rtVideoStreamingSegmentPacketSize");
        frameSize_ = ueAppModule_->registerSignal("rtVideoStreamingFrameSize");
        playoutBufferLength_ = ueAppModule_->registerSignal("rtVideoStreamingPlayoutBufferLength");
        playoutDelayTime_ = ueAppModule_->registerSignal("rtVideoStreamingPlayoutDelay");
        playoutDelayTimeAll_ = ueAppModule_->registerSignal("rtVideoStreamingPlayoutDelayAll");
        segmentLoss_ = ueAppModule_->registerSignal("rtVideoStreamingSegmentLoss");
        interArrTime_ = ueAppModule_->registerSignal("rtVideoStreamingInterArrivalTimeSegment");
        frameDisplayed_ = ueAppModule_->registerSignal("rtVideoStreamingFramesDisplayed");

        startSession_ = ueAppModule_->registerSignal("rtVideoStreamingStartSession");
        stopSession_ = ueAppModule_->registerSignal("rtVideoStreamingStopSession");
    }

    auto mecPk = pk->peekAtFront<RealTimeVideoStreamingAppPacket>();
    if(mecPk->getType() == START_RTVIDEOSTREAMING)
    {
        handleStartMessage(msg);
    }
    else if(mecPk->getType() == STOP_RTVIDEOSTREAMING)
    {
        handleStopMessage(msg);
    }
    else if(mecPk->getType() == START_RTVIDEOSTREAMING_SESSION)
    {
        handleSessionStartMessage(msg);
    }
    else if(mecPk->getType() == STOP_RTVIDEOSTREAMING_SESSION)
    {
        handleSessionStopMessage(msg);
    }
    else if(mecPk->getType() == RTVIDEOSTREAMING_SEGMENT)
    {
        processPacket(pk);
    }

    delete pk;
}

void MecRTVideoStreamingReceiver::handleSelfMessage(cMessage *msg)
{
    if(strcmp(msg->getName(), "displayFrame") == 0)
    {
      double percentage = 0.0;

      percentage = playoutFrame();

//      if(percentage >= correctFrameThreshold && isFirstFrame)
//      {
//          firstFrameDisplayedTimeStamp = simTime();
//          emit(firstFrameElapsedTime, (firstFrameDisplayedTimeStamp - videoRequestTimestamp));
//          isFirstFrame = false;
//      }


      /*
       * emit the time between the request and the display of the first frame available
       */

//      emit(framesDisplayed_, percentage);
      if(!stopped)
          scheduleAfter((1./fps), displayFrame);
      return;
    }
}


void MecRTVideoStreamingReceiver::handleStartMessage(cMessage* msg)
{
    EV << "MecRTVideoStreamingReceiver::handleStartMessage - START_RTVIDEOSTREAMING msg arrived from: " << ueAppAddress.str() << endl;
    auto pk = check_and_cast<Packet *>(msg);
    auto startPkt = pk->removeAtFront<StartRealTimeVideoStreamingAppPacket>();

    fps = startPkt->getFps();
    initialPlayoutDelay = par("initialPlayoutDelay");
    dropPackets_ = false;

    scheduleAfter(initialPlayoutDelay, displayFrame);
    startPkt->setType(START_RTVIDEOSTREAMING_ACK);

    inet::Packet* packet = new inet::Packet("RealTimeVideoStreamingAppPacket");
    packet->insertAtBack(startPkt);
    ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
}

void MecRTVideoStreamingReceiver::handleStopMessage(cMessage* msg)
{
    auto pk = check_and_cast<Packet *>(msg);
    auto stoptPkt = pk->removeAtFront<RealTimeVideoStreamingAppPacket>();

    EV << "MecRTVideoStreamingReceiver::handleStopMessage - STOP_RTVIDEOSTREAMING msg arrived" << endl;

    stopped = true;
//        if(displayFrame->isScheduled())
    cancelEvent(displayFrame);

    // clear structures
    playoutBuffer_.clear();
    firstFrameDisplayed = false;
    lastFrameDisplayed_ = -1;
    expectedFrameDisplayed_ = 0;

    stoptPkt->setType(STOP_RTVIDEOSTREAMING_ACK);

    inet::Packet* packet = new inet::Packet("RealTimeVideoStreamingAppPacket");
    packet->insertAtBack(stoptPkt);
    ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
}

void MecRTVideoStreamingReceiver::handleSessionStartMessage(cMessage* msg)
{
    auto pk = check_and_cast<Packet *>(msg);
    auto startPkt = pk->removeAtFront<StartSessionRealTimeVideoStreamingAppPacket>();

    EV << "MecRTVideoStreamingReceiver::handleStartSessionMessage - START_RTVIDEOSTREAMING_SESSION msg with session id [" << startPkt->getSessionId() << "] arrived" << endl;

    if(!stopped)
    {
        EV << "MecRTVideoStreamingReceiver::handleStartSessionMessage - session with id ["<< currentSessionId_ << "] is currently active. Discard msg" << endl;
        startPkt->setType(START_RTVIDEOSTREAMING_SESSION_NACK);

    }
    else
    {
        fps = startPkt->getFps();
        initialPlayoutDelay = par("initialPlayoutDelay");
        dropPackets_ = false;
        stopped = false;
        currentSessionId_ = startPkt->getSessionId();

        scheduleAfter(initialPlayoutDelay, displayFrame);
        startPkt->setType(START_RTVIDEOSTREAMING_SESSION_ACK);
        ueAppModule_->emit(startSession_, simTime());
    }

    inet::Packet* packet = new inet::Packet("RealTimeVideoStreamingAppPacket");
    packet->insertAtBack(startPkt);
    ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
}

void MecRTVideoStreamingReceiver::handleSessionStopMessage(cMessage* msg)
{
    auto pk = check_and_cast<Packet *>(msg);
    auto stoptPkt = pk->removeAtFront<StopSessionRealTimeVideoStreamingAppPacket>();

    EV << "MecRTVideoStreamingReceiver::handleSessionStopMessage - STOP_RTVIDEOSTREAMING_SESSION msg with session id [" << stoptPkt->getSessionId() << "] arrived" << endl;
    if(stopped || stoptPkt->getSessionId() != currentSessionId_)
    {
        if(stoptPkt->getSessionId() > currentSessionId_)
            EV << "MecRTVideoStreamingReceiver::handleSessionStopMessage - session with id ["<< stoptPkt->getSessionId() << "] was already stopped. Discard msg" << endl;
        else
            EV << "MecRTVideoStreamingReceiver::handleSessionStopMessage - The MecRTVideoStreamingReceiver is already stopped. Discard msg" << endl;
        stoptPkt->setType(STOP_RTVIDEOSTREAMING_SESSION_NACK);
    }
    else
    {
        stopped = true;
//        if(displayFrame->isScheduled())
        cancelEvent(displayFrame);
        ueAppModule_->emit(stopSession_, simTime());

        // clear structures
        playoutBuffer_.clear();
        firstFrameDisplayed = false;
        lastFrameDisplayed_ = -1;
        expectedFrameDisplayed_ = 0;

        stoptPkt->setType(STOP_RTVIDEOSTREAMING_SESSION_ACK);
    }


    inet::Packet* packet = new inet::Packet("RealTimeVideoStreamingAppPacket");
    packet->insertAtBack(stoptPkt);
    ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
}

double MecRTVideoStreamingReceiver::playoutFrame()
{
    /* It should be displayed the first frame in the map (ordered by timestamp).
     * This function gets the first frame in the map and checks if the playout time
     * corresponds with the actual time (or the difference is less than en epsilon that
     * takes into account roundings.
     */

    double percentage = 0.;
    int frameSize = 0;
    if(playoutBuffer_.empty())
    {
        /* The buffer is empty. There are no frames to be displayed
         *
         */
        EV << "MecRTVideoStreamingReceiver::playoutFrame() - the playout buffer is empty" << endl;
        lastFrameDisplayed_++;
    }
    else
    {
        auto firstFrame = playoutBuffer_.begin();
        // check the its playout time
        EV << "MecRTVideoStreamingReceiver::playoutFrame() - frame["<< firstFrame->second.frameNumber << "]" << endl;

        /**
         * This flag register the first available frame to be displayed.
         */
        if(firstFrameDisplayed == false)
        {
            firstFrameDisplayed = true;
            lastFrameDisplayed_ = firstFrame->second.frameNumber - 1;
            expectedFrameDisplayed_ = firstFrame->second.frameNumber;
        }

        if(firstFrame->second.frameNumber == expectedFrameDisplayed_)
        {
            EV << "tpCarProfilePayload99Receiver::playoutFrame - current frame size: " << firstFrame->second.currentSize << endl;
            EV << "tpCarProfilePayload99Receiver::playoutFrame - total frame size: " << firstFrame->second.frameSize << endl;
            percentage = (double)firstFrame->second.currentSize / firstFrame->second.frameSize;
            frameSize = firstFrame->second.currentSize ;
            EV << "MecRTVideoStreamingReceiver::playoutFrame() - frame with seqNumber [" << firstFrame->second.frameNumber << "] with percentage of " <<  percentage*100 << "%" << endl;

            int segmentPacketLost = firstFrame->second.numberOfFragments - firstFrame->second.numberOfFragmentsReceived;
            if(segmentPacketLost > 0)
            {
                ueAppModule_->emit(segmentLoss_, segmentPacketLost);
            }
            lastFrameDisplayed_ = firstFrame->second.frameNumber;
            playoutBuffer_.erase(firstFrame);
        }
        else if (firstFrame->second.frameNumber > expectedFrameDisplayed_)
        {
            // for now it is a debug
            EV << "The expected frame with number ["<<  expectedFrameDisplayed_ << "] is missing, the first frame number in the buffer has number [" << firstFrame->second.frameNumber  <<"]" << endl;
        }
        else if (firstFrame->second.frameNumber < expectedFrameDisplayed_)
        {
            // for now it is a debug
            throw cRuntimeError("The first frame in the buffer has a frame number [%d] lower than the expected one [%d]. This should not happen.",firstFrame->second.frameNumber, expectedFrameDisplayed_);
        }
    }

    expectedFrameDisplayed_++;
    ueAppModule_->emit(frameSize_, frameSize);
    ueAppModule_->emit(frameDisplayed_, percentage);
    return percentage;
}

void MecRTVideoStreamingReceiver::processPacket(Packet *packet)
{
    const auto& header = packet->popAtFront<RealTimeVideoStreamingAppSegmentHeader>();
    simtime_t creationTime = header->getTag<CreationTimeTag>()->getCreationTime();

    if(dropPackets_)
    {
        EV << "MecRTVideoStreamingReceiver::processPacket - application parameters are not configured, yet. Discarding this packet" << endl;
        delete packet;
        return;
    }


    ueAppModule_->emit(e2eDelaySegment_, simTime() - creationTime);
    if(lastfragment_ != 0)
    {
        ueAppModule_->emit(interArrTime_, simTime() - lastfragment_);
    }


    lastfragment_ = simTime();
    int frameNumber = header->getFrameNumber();
    ueAppModule_->emit(segmentSize_, header->getPayloadLength());
    EV << "MecRTVideoStreamingReceiver::processPacket - received packet with seqNum [" << header->getSequenceNumber() << "] of frame [" << frameNumber << "] and delay " << simTime()- creationTime << " s" << endl;

    /*
     * dropFrames flag is false, so take into account the frame.
     * If the simulation time is **lower** than the expected start of the playout video:
     *  - buffer the frame as normal.
     *
     * If the simulation time is **higher** than the expected start of the playout video
     * AND no frames arrived before:
     *  - check if the frame is older than 50ms, and if true drop the packet, If it is not
     *    buffer the frames as normal. This is useful to start displaying only new frames, to
     *    support a more real time approach     *
     */


    if(frameNumber <= lastFrameDisplayed_)
    {
        EV << "MecRTVideoStreamingReceiver::processPacket - received packet related to frame [" << frameNumber << "], but the last frame displayed is [" << lastFrameDisplayed_ << "] discarded" << endl;
        return;
    }

    else
    {
        // check if the frame structure is already present in the buffer. Or create a new one
        auto frameStruct = playoutBuffer_.find(frameNumber);
        if(frameStruct == playoutBuffer_.end())
        {
            ReceivingFrameStatus newFrame;
            newFrame.frameSize = header->getFrameLength();
            newFrame.currentSize = header->getPayloadLength();
            //newFrame.playoutTime = simTime().dbl() + playoutTime;
            newFrame.frameNumber = frameNumber;
            newFrame.numberOfFragments = header->getTotalFrags();
            newFrame.numberOfFragmentsReceived = 1;

            playoutBuffer_.insert({frameNumber, newFrame});
            int size = playoutBuffer_.size();
            ueAppModule_->emit(playoutBufferLength_, size);

            EV << "MecRTVideoStreamingReceiver::processPacket - store new frame ["<< newFrame.frameNumber << "], packet with seqNum [" << header->getSequenceNumber() << "] contains "
                    << header->getPayloadLength() << " bytes.\nThe current size of the frame is " <<  newFrame.currentSize << " of "
                           << newFrame.frameSize << " bytes" << endl;
        }
        else
        {
            frameStruct->second.currentSize += header->getPayloadLength();
            frameStruct->second.numberOfFragmentsReceived += 1;

            EV << "MecRTVideoStreamingReceiver::processPacket - packet with seqNum [" << header->getSequenceNumber() << "] relative to frame ["
               << frameStruct->second.frameNumber << "] contains " << header->getPayloadLength() << " bytes.\nThe current size of the frame is " <<  frameStruct->second.currentSize << " of "
               << frameStruct->second.frameSize << " bytes" << endl;

            if(frameStruct->second.currentSize > frameStruct->second.frameSize)
            {
                // just a debug
                // this should not happen
                throw cRuntimeError("MecRTVideoStreamingReceiver::processPacket - frame has a current size [%d], bigger than the expected [%d]", frameStruct->second.currentSize, frameStruct->second.frameSize);
            }
        }
    }

}


