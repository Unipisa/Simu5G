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

#include "stack/backgroundTrafficGenerator/generators/TrafficGeneratorBase.h"

#include <inet/mobility/contract/IMobility.h>

#include "stack/backgroundTrafficGenerator/generators/RtxNotification_m.h"

namespace simu5g {

Define_Module(TrafficGeneratorBase);

// statistics
simsignal_t TrafficGeneratorBase::bgMeasuredSinrDlSignal_ = registerSignal("bgMeasuredSinrDl");
simsignal_t TrafficGeneratorBase::bgMeasuredSinrUlSignal_ = registerSignal("bgMeasuredSinrUl");
simsignal_t TrafficGeneratorBase::bgAverageCqiDlSignal_ = registerSignal("bgAverageCqiDl");
simsignal_t TrafficGeneratorBase::bgAverageCqiUlSignal_ = registerSignal("bgAverageCqiUl");
simsignal_t TrafficGeneratorBase::bgHarqErrorRateDlSignal_ = registerSignal("bgHarqErrorRateDl");
simsignal_t TrafficGeneratorBase::bgHarqErrorRateUlSignal_ = registerSignal("bgHarqErrorRateUl");

TrafficGeneratorBase::TrafficGeneratorBase()
{
    selfSource_[DL] = selfSource_[UL] = nullptr;
    bufferedBytes_[DL] = bufferedBytes_[UL] = 0;
    bufferedBytesRtx_[DL] = bufferedBytesRtx_[UL] = 0;
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
    if (stage == inet::INITSTAGE_LOCAL) {
        bgUeIndex_ = getParentModule()->getIndex();

        // calculating traffic starting time
        startTime_[DL] = par("startTimeDl");
        startTime_[UL] = par("startTimeUl");

        headerLen_ = par("headerLen");
        txPower_ = par("txPower");

        rtxRate_[DL] = par("rtxRateDl");
        rtxRate_[UL] = par("rtxRateUl");

        rtxDelay_[DL] = par("rtxDelayDl");
        rtxDelay_[UL] = par("rtxDelayUl");

        bgTrafficManager_.reference(this, "backgroundTrafficManagerModule", true);

        if (startTime_[DL] >= 0.0) {
            trafficEnabled_[DL] = true;
            selfSource_[DL] = new cMessage("selfSourceDl");
            scheduleAt(simTime() + startTime_[DL], selfSource_[DL]);
        }

        if (startTime_[UL] >= 0.0) {
            trafficEnabled_[UL] = true;
            selfSource_[UL] = new cMessage("selfSourceUl");
            scheduleAt(simTime() + startTime_[UL], selfSource_[UL]);
        }

        cqiMeanDl_ = par("cqiMeanDl");
        cqiStddevDl_ = par("cqiStddevDl");
        cqiMeanUl_ = par("cqiMeanUl");
        cqiStddevUl_ = par("cqiStddevUl");

        enablePeriodicCqiUpdate_ = par("enablePeriodicCqiUpdate");
        useProbabilisticCqi_ = par("useProbabilisticCqi");
        computeAvgInterference_ = par("computeAvgInterference");
        if (enablePeriodicCqiUpdate_) {
            fbPeriod_ = (simtime_t)(int(par("fbPeriod")) * TTI); // TTI -> seconds
            fbSource_ = new cMessage("fbSource");
            scheduleAt(simTime(), fbSource_);
        }
        else if (!computeAvgInterference_) {
            // use fixed CQI given as parameters

            double cqiDl = normal(cqiMeanDl_, cqiStddevDl_);
            if (cqiDl > 15)
                cqi_[DL] = 15;
            else if (cqiDl < 2)
                cqi_[DL] = 2;
            else
                cqi_[DL] = floor(cqiDl);

            double cqiUl = normal(cqiMeanUl_, cqiStddevUl_);
            if (cqiUl > 15)
                cqi_[UL] = 15;
            else if (cqiUl < 2)
                cqi_[UL] = 2;
            else
                cqi_[UL] = floor(cqiUl);
        }

        // register to get a notification when positions change
        getParentModule()->subscribe(inet::IMobility::mobilityStateChangedSignal, this);
        positionUpdated_ = true;
    }
}

void TrafficGeneratorBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        // update measurements every feedback period
        if (msg == fbSource_) {
            updateMeasurements();
            scheduleAt(simTime() + fbPeriod_, fbSource_);
            return;
        }

        // if periodic CQI update is disabled, and CQI was not estimated using average interference, then update SINR when the UE changed its position
        if (!enablePeriodicCqiUpdate_ && !computeAvgInterference_ && positionUpdated_)
            updateMeasurements();

        if (msg == selfSource_[DL]) {
            unsigned int genBytes = generateTraffic(DL);
            if (genBytes == bufferedBytes_[DL]) {
                // the UE has become active, signal to the manager
                bgTrafficManager_->notifyBacklog(bgUeIndex_, DL);
            }

            // generate new traffic in 'offset' seconds
            simtime_t offset = getNextGenerationTime(DL);
            scheduleAt(simTime() + offset, selfSource_[DL]);
        }
        else if (msg == selfSource_[UL]) {
            unsigned int genBytes = generateTraffic(UL);
            if (genBytes == bufferedBytes_[UL]) {
                // the UE has become active, signal to the manager
                bgTrafficManager_->notifyBacklog(bgUeIndex_, UL);
            }

            // generate new traffic in 'offset' seconds
            simtime_t offset = getNextGenerationTime(UL);
            scheduleAt(simTime() + offset, selfSource_[UL]);
        }
        else if (!strcmp(msg->getName(), "rtxNotification")) {
            RtxNotification *rtxNotification = check_and_cast<RtxNotification *>(msg);
            Direction dir = (Direction)rtxNotification->getDirection();
            bufferedBytesRtx_[dir] += rtxNotification->getBytes();
            if (rtxNotification->getBytes() == bufferedBytesRtx_[dir]) {
                // the UE has become active, signal to the manager
                bgTrafficManager_->notifyBacklog(bgUeIndex_, dir, true);
            }
            delete rtxNotification;
        }
    }
}

void TrafficGeneratorBase::updateMeasurements()
{
    if (useProbabilisticCqi_) {
        if (trafficEnabled_[DL]) {
            double cqiDl = normal(cqiMeanDl_, cqiStddevDl_);

            if (cqiDl > 15)
                cqi_[DL] = 15;
            else if (cqiDl < 2)
                cqi_[DL] = 2;
            else
                cqi_[DL] = floor(cqiDl + 0.5);
        }

        if (trafficEnabled_[UL]) {
            double cqiUl = normal(cqiMeanUl_, cqiStddevUl_);
            if (cqiUl > 15)
                cqi_[UL] = 15;
            else if (cqiUl < 2)
                cqi_[UL] = 2;
            else
                cqi_[UL] = floor(cqiUl + 0.5);
        }
    }
    else {
        if (trafficEnabled_[DL])
            cqi_[DL] = bgTrafficManager_->computeCqi(bgUeIndex_, DL, pos_);

        if (trafficEnabled_[UL])
            cqi_[UL] = bgTrafficManager_->computeCqi(bgUeIndex_, UL, pos_, txPower_);
    }

    positionUpdated_ = false;
}

unsigned int TrafficGeneratorBase::generateTraffic(Direction dir)
{
    unsigned int dataLen = (dir == DL) ? par("packetSizeDl") : par("packetSizeUl");
    bufferedBytes_[dir] += (dataLen + headerLen_);
    return dataLen + headerLen_;
}

simtime_t TrafficGeneratorBase::getNextGenerationTime(Direction dir)
{
    // TODO differentiate RNG based on direction

    simtime_t offset = (dir == DL) ? par("periodDl") : par("periodUl");
    return offset;
}

unsigned int TrafficGeneratorBase::getBufferLength(Direction dir, bool rtx)
{
    if (!rtx)
        return bufferedBytes_[dir];
    else
        return bufferedBytesRtx_[dir];
}

void TrafficGeneratorBase::setCqiFromSinr(double sinr, Direction dir)
{
    cqi_[dir] = bgTrafficManager_->computeCqiFromSinr(sinr);
}

Cqi TrafficGeneratorBase::getCqi(Direction dir)
{
    return cqi_[dir];
}

unsigned int TrafficGeneratorBase::consumeBytes(int bytes, Direction dir, bool rtx)
{
    Enter_Method_Silent("TrafficGeneratorBase::consumeBytes");

    if (dir != DL && dir != UL)
        throw cRuntimeError("TrafficGeneratorBase::consumeBytes - unrecognized direction: %d", dir);

    if (!rtx) {
        if (bytes > bufferedBytes_[dir])
            bytes = bufferedBytes_[dir];

        bufferedBytes_[dir] -= bytes;
    }
    else {
        if (bytes > bufferedBytesRtx_[dir])
            bytes = bufferedBytesRtx_[dir];

        bufferedBytesRtx_[dir] -= bytes;
    }

    // this simulates a transmission, so emit CQI statistic
    if (dir == DL)
        emit(bgAverageCqiDlSignal_, (long)cqi_[DL]);
    else
        emit(bgAverageCqiUlSignal_, (long)cqi_[UL]);

    // "schedule" a retransmission with the given probability
    double err = uniform(0.0, 1.0);
    if (err < rtxRate_[dir]) {
        RtxNotification *rtxNotification = new RtxNotification("rtxNotification");
        rtxNotification->setDirection(dir);
        rtxNotification->setBytes(bytes);
        scheduleAt(NOW + rtxDelay_[dir], rtxNotification);

        if (dir == DL)
            emit(bgHarqErrorRateDlSignal_, 1.0);
        else
            emit(bgHarqErrorRateUlSignal_, 1.0);
    }
    else {
        if (dir == DL)
            emit(bgHarqErrorRateDlSignal_, 0.0);
        else
            emit(bgHarqErrorRateUlSignal_, 0.0);
    }

    return (!rtx) ? bufferedBytes_[dir] : bufferedBytesRtx_[dir];
}

void TrafficGeneratorBase::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *)
{
    if (signalID == inet::IMobility::mobilityStateChangedSignal) {
        inet::IMobility *mobility = check_and_cast<inet::IMobility *>(obj);
        pos_ = mobility->getCurrentPosition();
        positionUpdated_ = true;
    }
}

void TrafficGeneratorBase::collectMeasuredSinr(double sample, Direction dir)
{
    if (dir == DL)
        emit(bgMeasuredSinrDlSignal_, sample);
    else
        emit(bgMeasuredSinrUlSignal_, sample);
}

} //namespace

