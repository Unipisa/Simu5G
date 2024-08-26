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

#include "nodes/backgroundCell/BackgroundScheduler.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(BackgroundScheduler);

// statistics
simsignal_t BackgroundScheduler::bgAvgServedBlocksDlSignal_ = registerSignal("bgAvgServedBlocksDl");
simsignal_t BackgroundScheduler::bgAvgServedBlocksUlSignal_ = registerSignal("bgAvgServedBlocksUl");

void BackgroundScheduler::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        // set TX power and angle

        txPower_ = par("txPower");

        std::string txDir = par("txDirection");
        txDirection_ = static_cast<TxDirectionType>(cEnum::get("simu5g::TxDirectionType")->lookup(txDir.c_str()));
        switch (txDirection_) {
            case OMNI: txAngle_ = 0.0;
                break;
            case ANISOTROPIC: txAngle_ = par("txAngle");
                break;
            default: throw cRuntimeError("unknown txDirection: '%s'", txDir.c_str());
        }

        isNr_ = par("isNr");
        numerologyIndex_ = par("numerologyIndex");
        carrierFrequency_ = par("carrierFrequency").doubleValue();
        numBands_ = par("numBands");

        // initialize band status structures
        bandStatus_[UL].resize(numBands_, 0);
        prevBandStatus_[UL].resize(numBands_, 0);
        bandStatus_[DL].resize(numBands_, 0);
        prevBandStatus_[DL].resize(numBands_, 0);
        ulPrevBandAllocation_.resize(numBands_, NODEID_NONE);
        ulBandAllocation_.resize(numBands_, NODEID_NONE);

        // TODO: if BackgroundScheduler interference is disabled, do not send selfMessages
        // Start TTI tick
        ttiTick_ = new cMessage("ttiTick_");
        ttiTick_->setSchedulingPriority(1);        // TTI TICK after other messages
        ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(numerologyIndex_);
        scheduleAt(NOW + ttiPeriod_, ttiTick_);

        // register to get a notification when position changes
        getParentModule()->subscribe(inet::IMobility::mobilityStateChangedSignal, this);
    }
    if (stage == inet::INITSTAGE_LOCAL + 1) {
        binder_.reference(this, "binderModule", true);

        // add this cell to the binder
        id_ = binder_->addBackgroundScheduler(this, carrierFrequency_);

        bgTrafficManager_.reference(this, "trafficManagerModule", true);
        bgTrafficManager_->setCarrierFrequency(carrierFrequency_);

        bgChannelModel_.reference(this, "channelModelModule", true);
        bgChannelModel_->setCarrierFrequency(carrierFrequency_);
    }
}

void BackgroundScheduler::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        updateAllocation(UL);
        updateAllocation(DL);

        scheduleAt(NOW + ttiPeriod_, msg);
        return;
    }
}

void BackgroundScheduler::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *)
{
    if (signalID == inet::IMobility::mobilityStateChangedSignal) {
        // this is a HACK to prevent the bgBaseStation from changing its position when its background UEs change theirs
        // this would happen because the background UEs' mobility module is a submodule of the bgBaseStation
        if (this->getParentModule() != source->getParentModule())
            return;

        inet::IMobility *mobility = check_and_cast<inet::IMobility *>(obj);
        pos_ = mobility->getCurrentPosition();
    }
}

void BackgroundScheduler::updateAllocation(Direction dir)
{
    EV << "----- BACKGROUND CELL ALLOCATION UPDATE -----" << endl;

    resetAllocation(dir);

    unsigned int b = 0;
    int bgUeIndex;
    int bytesPerBlock;
    MacNodeId bgUeId;

    // --- schedule RAC (UL only) --- //
    if (dir == UL) {
        std::list<MacNodeId> servedRac;

        // handle RAC
        auto rit = bgTrafficManager_->getWaitingForRacUesBegin();
        auto ret = bgTrafficManager_->getWaitingForRacUesEnd();

        for ( ; rit != ret; ++rit) {
            // if there is still space
            if (b >= numBands_) {
                // space terminated
                EV << "BackgroundScheduler::updateAllocation - space ended" << endl;
                break;
            }

            bgUeIndex = *rit;
            bgUeId = BGUE_MIN_ID + bgUeIndex;

            EV << NOW << " BackgroundScheduler::updateAllocation - dir[" << dirToA(dir) << "] band[" << b << "] - allocated to ue[" << bgUeId << "]" << endl;

            // allocate one block
            bandStatus_[dir][b] = 1;
            ulBandAllocation_[b] = bgUeId;
            b++;

            servedRac.push_back(bgUeId);

            if (b >= numBands_) {
                EV << "BackgroundScheduler::updateAllocation - space ended" << endl;
                EV << "----- END BACKGROUND CELL ALLOCATION UPDATE -----" << endl;
                return;
            }
        }

        while (!servedRac.empty()) {
            // notify the traffic manager that the RAC for this UE has been served
            bgTrafficManager_->racHandled(servedRac.front());
            servedRac.pop_front();
        }
    }

    // --- schedule retransmissions --- //
    std::map<MacNodeId, unsigned int> rtxScheduledBgUes;
    auto rit = bgTrafficManager_->getBackloggedUesBegin(dir, true);
    auto ret = bgTrafficManager_->getBackloggedUesEnd(dir, true);
    for ( ; rit != ret; ++rit) {
        // if there is still space
        if (b >= numBands_) {
            // space terminated
            EV << "BackgroundScheduler::updateAllocation - space ended" << endl;
            break;
        }

        bgUeIndex = *rit;
        bgUeId = BGUE_MIN_ID + bgUeIndex;

        bytesPerBlock = bgTrafficManager_->getBackloggedUeBytesPerBlock(bgUeId, dir);

        unsigned int buffer = bgTrafficManager_->getBackloggedUeBuffer(bgUeId, dir, true);
        unsigned int blocks = ceil((double)buffer / bytesPerBlock);
        unsigned int allocatedBlocks = 0;
        while (b < numBands_ && blocks > 0) {
            bandStatus_[dir][b] = 1;
            if (dir == UL)
                ulBandAllocation_[b] = bgUeId;

            EV << NOW << " BackgroundScheduler::updateAllocation - dir[" << dirToA(dir) << "] band[" << b << "] - allocated to ue[" << bgUeId << "] for rtx" << endl;

            blocks--;
            b++;
            allocatedBlocks++;
        }

        if (allocatedBlocks > 0) {
            unsigned int allocatedBytes = allocatedBlocks * bytesPerBlock;
            rtxScheduledBgUes[bgUeId] = allocatedBytes;
        }
    }

    // notify the traffic manager
    for (auto & rtxScheduledBgUe : rtxScheduledBgUes)
        bgTrafficManager_->consumeBackloggedUeBytes(rtxScheduledBgUe.first, rtxScheduledBgUe.second, dir, true);

    // sort backlogged UEs (MaxC/I)
    ScoreList score;
    MacCid bgCid;
    auto it = bgTrafficManager_->getBackloggedUesBegin(dir);
    auto et = bgTrafficManager_->getBackloggedUesEnd(dir);
    for ( ; it != et; ++it) {
        // if there is still space
        if (b >= numBands_) {
            // space terminated
            EV << "BackgroundScheduler::updateAllocation - space ended" << endl;
            break;
        }

        bgUeIndex = *it;
        bgUeId = BGUE_MIN_ID + bgUeIndex;

        // if the BG UE has been already scheduled for rtx, skip it
        if (rtxScheduledBgUes.find(bgUeId) != rtxScheduledBgUes.end())
            continue;

        // the cid for a background UE is a 32bit integer composed as:
        // - the most significant 16 bits are set to the background UE id (BGUE_MIN_ID+index)
        // - the least significant 16 bits are set to 0 (lcid=0)
        bgCid = num(bgUeId) << 16;

        bytesPerBlock = bgTrafficManager_->getBackloggedUeBytesPerBlock(bgUeId, dir);

        ScoreDesc bgDesc(bgCid, bytesPerBlock);
        score.push(bgDesc);
    }

    // Schedule the connections in score order.
    while (!score.empty()) {
        // if there is still space
        if (b >= numBands_) {
            // space terminated
            EV << "BackgroundScheduler::updateAllocation - space ended" << endl;
            break;
        }

        // Pop the top connection from the list.
        ScoreDesc current = score.top();
        bgUeId = MacNodeId(current.x_ >> 16);
        bytesPerBlock = current.score_;

        unsigned int buffer = bgTrafficManager_->getBackloggedUeBuffer(bgUeId, dir);
        unsigned int blocks = ceil((double)buffer / bytesPerBlock);
        unsigned int allocatedBlocks = 0;
        while (b < numBands_ && blocks > 0) {
            bandStatus_[dir][b] = 1;
            if (dir == UL)
                ulBandAllocation_[b] = bgUeId;

            EV << NOW << " BackgroundScheduler::updateAllocation - dir[" << dirToA(dir) << "] band[" << b << "] - allocated to ue[" << bgUeId << "]" << endl;

            blocks--;
            b++;
            allocatedBlocks++;
        }

        unsigned int allocatedBytes = allocatedBlocks * bytesPerBlock;
        bgTrafficManager_->consumeBackloggedUeBytes(bgUeId, allocatedBytes, dir);

        // remove UE from the list
        score.pop();
    }

    // emit statistics
    if (dir == DL)
        emit(bgAvgServedBlocksDlSignal_, (long)b);
    else
        emit(bgAvgServedBlocksUlSignal_, (long)b);

    EV << "----- END BACKGROUND CELL ALLOCATION UPDATE -----" << endl;
}

void BackgroundScheduler::resetAllocation(Direction dir)
{
    for (unsigned int i = 0; i < numBands_; i++) {
        prevBandStatus_[dir][i] = bandStatus_[dir][i];
        bandStatus_[dir][i] = 0;
        if (dir == UL) {
            ulPrevBandAllocation_[i] = ulBandAllocation_[i];
            ulBandAllocation_[i] = NODEID_NONE;
        }
    }
}

TrafficGeneratorBase *BackgroundScheduler::getBandInterferingUe(int band)
{
    MacNodeId bgUeId = ulBandAllocation_[band];
    return bgTrafficManager_->getTrafficGenerator(bgUeId);
}

TrafficGeneratorBase *BackgroundScheduler::getPrevBandInterferingUe(int band)
{
    MacNodeId bgUeId = ulPrevBandAllocation_[band];
    return bgTrafficManager_->getTrafficGenerator(bgUeId);
}

} //namespace

