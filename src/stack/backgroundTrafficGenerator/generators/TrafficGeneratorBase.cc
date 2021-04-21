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

#include "inet/mobility/contract/IMobility.h"
#include "TrafficGeneratorBase.h"

Define_Module(TrafficGeneratorBase);

TrafficGeneratorBase::TrafficGeneratorBase()
{
    fbSource_ = nullptr;
    selfSource_[DL] = selfSource_[UL] = nullptr;
    bufferedBytes_[DL] = bufferedBytes_[UL] = 0;
    trafficEnabled_[DL] = trafficEnabled_[UL] = false;
}

TrafficGeneratorBase::~TrafficGeneratorBase()
{
    cancelAndDelete(selfSource_[DL]);
    cancelAndDelete(selfSource_[UL]);
    cancelAndDelete(fbSource_);
}

void TrafficGeneratorBase::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
    {
        bgUeIndex_ = getParentModule()->getIndex();

        // calculating traffic starting time
        startTime_[DL] = par("startTimeDl");
        startTime_[UL] = par("startTimeUl");

        headerLen_ = par("headerLen");

        txPower_ = par("txPower");

        bgTrafficManager_ = check_and_cast<BackgroundTrafficManager*>(getParentModule()->getParentModule()->getSubmodule("manager"));

        if (startTime_[DL] >= 0.0)
        {
            trafficEnabled_[DL] = true;
            selfSource_[DL] = new cMessage("selfSourceDl");
            scheduleAt(simTime()+startTime_[DL], selfSource_[DL]);
        }

        if (startTime_[UL] >= 0.0)
        {
            trafficEnabled_[UL] = true;
            selfSource_[UL] = new cMessage("selfSourceUl");
            scheduleAt(simTime()+startTime_[UL], selfSource_[UL]);
        }

        enablePeriodicCqiUpdate_ = getAncestorPar("enablePeriodicCqiUpdate");
        if (enablePeriodicCqiUpdate_)
        {
            fbPeriod_ = (simtime_t)(int(par("fbPeriod")) * TTI); // TTI -> seconds
            fbSource_ = new cMessage("fbSource");
            scheduleAt(simTime(), fbSource_);
        }

        useAvgInterference_ = getAncestorPar("useAvgInterference");

        // register to get a notification when position changes
        getParentModule()->subscribe(inet::IMobility::mobilityStateChangedSignal, this);
        positionUpdated_ = true;
    }
}

void TrafficGeneratorBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        // update measurements every feedback period
        if (!strcmp(msg->getName(), "fbSource"))
        {
            updateMeasurements();
            scheduleAt(simTime()+fbPeriod_, fbSource_);
            return;
        }

        // if periodic CQI updateis disabled, and CQI was not estimated using avg interference, then update SINR when the UE changed its position
        if (!enablePeriodicCqiUpdate_ && !useAvgInterference_ && positionUpdated_)
            updateMeasurements();

        if (!strcmp(msg->getName(), "selfSourceDl"))
        {
            unsigned int genBytes = generateTraffic(DL);
            if (genBytes == bufferedBytes_[DL])
            {
                // the UE has become active, signal to the manager
                bgTrafficManager_->notifyBacklog(bgUeIndex_, DL);
            }

            // generate new traffic in 'offset' seconds
            simtime_t offset = getNextGenerationTime(DL);
            scheduleAt(simTime()+offset, selfSource_[DL]);
        }
        else if (!strcmp(msg->getName(), "selfSourceUl"))
        {
            unsigned int genBytes = generateTraffic(UL);
            if (genBytes == bufferedBytes_[UL])
            {
                // the UE has become active, signal to the manager
                bgTrafficManager_->notifyBacklog(bgUeIndex_, UL);
            }

            // generate new traffic in 'offset' seconds
            simtime_t offset = getNextGenerationTime(UL);
            scheduleAt(simTime()+offset, selfSource_[UL]);
        }
    }
}

void TrafficGeneratorBase::updateMeasurements()
{
    if (trafficEnabled_[DL])
        cqi_[DL] = bgTrafficManager_->computeCqi(bgUeIndex_, DL, pos_);

    if (trafficEnabled_[UL])
        cqi_[UL] = bgTrafficManager_->computeCqi(bgUeIndex_, UL, pos_, txPower_);

    std::cout << NOW << "TrafficGeneratorBase::updateMeasurements - bgUe " << bgUeIndex_ << " cqiDl[" << cqi_[DL] << "] cqiUl[" << cqi_[UL] << "] "<< endl;

    positionUpdated_ = false;
}

unsigned int TrafficGeneratorBase::generateTraffic(Direction dir)
{
    unsigned int dataLen = (dir == DL) ? par("packetSizeDl") : par("packetSizeUl");
    bufferedBytes_[dir] += (dataLen + headerLen_);
    return (dataLen + headerLen_);
}

simtime_t TrafficGeneratorBase::getNextGenerationTime(Direction dir)
{
    // TODO differentiate RNG based on direction

    simtime_t offset = (dir == DL) ? par("periodDl") : par("periodUl");
    return offset;
}

unsigned int TrafficGeneratorBase::getBufferLength(Direction dir)
{
    return bufferedBytes_[dir];
}

void TrafficGeneratorBase::setCqiFromSinr(double sinr, Direction dir)
{
    cqi_[dir] = bgTrafficManager_->computeCqiFromSinr(sinr);
}


Cqi TrafficGeneratorBase::getCqi(Direction dir)
{
    return cqi_[dir];
}

unsigned int TrafficGeneratorBase::consumeBytes(int bytes, Direction dir)
{
    if (dir != DL && dir != UL)
       throw cRuntimeError("TrafficGeneratorBase::consumeBytes - unrecognized direction: %d" , dir);

    if (bytes > bufferedBytes_[dir])
        bytes = bufferedBytes_[dir];

    bufferedBytes_[dir] -= bytes;

    return bufferedBytes_[dir];
}


void TrafficGeneratorBase::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *)
{
    if (signalID == inet::IMobility::mobilityStateChangedSignal)
    {
        inet::IMobility *mobility = check_and_cast<inet::IMobility*>(obj);
        pos_ = mobility->getCurrentPosition();
        positionUpdated_ = true;
    }
}
