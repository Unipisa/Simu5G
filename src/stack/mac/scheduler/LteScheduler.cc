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

#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/mac/scheduler/LteSchedulerEnbUl.h"

using namespace omnetpp;

void LteScheduler::setEnbScheduler(LteSchedulerEnb* eNbScheduler)
{
    eNbScheduler_ = eNbScheduler;
    direction_ = eNbScheduler_->direction_;
    mac_ = eNbScheduler_->mac_;
}

void LteScheduler::setCarrierFrequency(double carrierFrequency)
{
    carrierFrequency_ = carrierFrequency;
}

void LteScheduler::initializeBandLimit()
{
    bandLimit_ = mac_->getCellInfo()->getCarrierBandLimit(carrierFrequency_);

    // initialize band limits to be used on each slot for retransmissions
    // and grant requests
    BandLimitVector::iterator it = bandLimit_->begin();
    for (; it != bandLimit_->end(); ++it)
    {
        BandLimit elem; // TODO to be moved outside the for ?
        elem.band_ = it->band_;
        elem.limit_ = it->limit_;

        slotRacBandLimit_.push_back(elem);
        slotRtxBandLimit_.push_back(elem);
        slotReqGrantBandLimit_.push_back(elem);
    }

//    // === DEBUG === //
//    EV << "LteScheduler::initializeBandLimit - Set Band Limit for this carrier: " << endl;
//    for (it = bandLimit_->begin(); it != bandLimit_->end(); ++it)
//    {
//        EV << " - Band[" << it->band_ << "] limit[";
//        for (unsigned int cw=0; cw < it->limit_.size(); cw++)
//        {
//            EV << it->limit_[cw] << ", ";
//        }
//        EV << "]" << endl;
//    }
//    // === END DEBUG === //
}


void LteScheduler::initializeSchedulerPeriodCounter(NumerologyIndex maxNumerologyIndex)
{
    // 2^(maxNumerologyIndex - numerologyIndex)
    maxSchedulingPeriodCounter_ = 1 << (maxNumerologyIndex - numerologyIndex_);
    currentSchedulingPeriodCounter_ = maxSchedulingPeriodCounter_ - 1;
}

unsigned int LteScheduler::decreaseSchedulerPeriodCounter()
{
    unsigned int ret = currentSchedulingPeriodCounter_;
    if (currentSchedulingPeriodCounter_ == 0) // reset
        currentSchedulingPeriodCounter_ = maxSchedulingPeriodCounter_ - 1;
    else
        currentSchedulingPeriodCounter_--;
    return ret;
}


unsigned int LteScheduler::requestGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, BandLimitVector* bandLim)
{
    if (bandLim == NULL)
    {
        // reset the band limit vector used for requesting grants
        for (unsigned int i = 0; i < bandLimit_->size(); i++)
        {
            // copy the element
            slotReqGrantBandLimit_[i].band_ = bandLimit_->at(i).band_;
            slotReqGrantBandLimit_[i].limit_ = bandLimit_->at(i).limit_;
        }
        bandLim = &slotReqGrantBandLimit_;
    }

//    // === DEBUG === //
//    EV << "LteScheduler::requestGrant - Set Band Limit for this carrier: " << endl;
//    BandLimitVector::iterator it = bandLim->begin();
//    for (; it != bandLim->end(); ++it)
//    {
//        EV << " - Band[" << it->band_ << "] limit[";
//        for (int cw=0; cw < it->limit_.size(); cw++)
//        {
//            EV << it->limit_[cw] << ", ";
//        }
//        EV << "]" << endl;
//    }
//    // === END DEBUG === //

    return eNbScheduler_->scheduleGrant(cid, bytes, terminate, active, eligible, carrierFrequency_, bandLim);
}

unsigned int LteScheduler::requestGrantBackground(MacCid bgCid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, BandLimitVector* bandLim)
{
    if (bandLim == NULL)
    {
        // reset the band limit vector used for requesting grants
        for (unsigned int i = 0; i < bandLimit_->size(); i++)
        {
            // copy the element
            slotReqGrantBandLimit_[i].band_ = bandLimit_->at(i).band_;
            slotReqGrantBandLimit_[i].limit_ = bandLimit_->at(i).limit_;
        }
        bandLim = &slotReqGrantBandLimit_;
    }

    return eNbScheduler_->scheduleGrantBackground(bgCid, bytes, terminate, active, eligible, carrierFrequency_, bandLim);
}

bool LteScheduler::scheduleRetransmissions()
{
    // step 1: schedule retransmissions, if any, for foreground UEs
    // step 2: schedule retransmissions, if any and if there is still space, for background UEs

    bool spaceEnded = false;
    bool skip = false;
    // optimization: do not call rtxschedule if no process is ready for rtx for this carrier
    if (eNbScheduler_->direction_ == DL && mac_->getProcessForRtx(carrierFrequency_, DL) == 0)
        skip = true;
    if (eNbScheduler_->direction_ == UL && mac_->getProcessForRtx(carrierFrequency_, UL) == 0 && mac_->getProcessForRtx(carrierFrequency_, D2D) == 0)
        skip = true;

    if (!skip)
    {
        // reset the band limit vector used for retransmissions
        // TODO do this only when it was actually used in previous slot
        for (unsigned int i = 0; i < bandLimit_->size(); i++)
        {
            // copy the element
            slotRtxBandLimit_[i].band_ = bandLimit_->at(i).band_;
            slotRtxBandLimit_[i].limit_ = bandLimit_->at(i).limit_;
        }
        spaceEnded = eNbScheduler_->rtxschedule(carrierFrequency_, &slotRtxBandLimit_);
    }

    if (!spaceEnded)
    {
        // check if there are backlogged retransmissions for background UEs
        BackgroundTrafficManager* bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency_);
        std::list<int>::const_iterator it = bgTrafficManager->getBackloggedUesBegin(eNbScheduler_->direction_, true),
                                       et = bgTrafficManager->getBackloggedUesEnd(eNbScheduler_->direction_, true);
        if (it != et)
        {
            // if the bandlimit was not reset for foreground UEs, do it here
            if (skip)
            {
                // reset the band limit vector used for retransmissions
                // TODO do this only when it was actually used in previous slot
                for (unsigned int i = 0; i < bandLimit_->size(); i++)
                {
                    // copy the element
                    slotRtxBandLimit_[i].band_ = bandLimit_->at(i).band_;
                    slotRtxBandLimit_[i].limit_ = bandLimit_->at(i).limit_;
                }
            }
            spaceEnded = eNbScheduler_->rtxscheduleBackground(carrierFrequency_, &slotRtxBandLimit_);
        }
    }
    return spaceEnded;
}

bool LteScheduler::scheduleRacRequests()
{
    // reset the band limit vector used for rac
    // TODO do this only when it was actually used in previous slot
    for (unsigned int i = 0; i < bandLimit_->size(); i++)
    {
        // copy the element
        slotRacBandLimit_[i].band_ = bandLimit_->at(i).band_;
        slotRacBandLimit_[i].limit_ = bandLimit_->at(i).limit_;
    }

    return eNbScheduler_->racschedule(carrierFrequency_, &slotRacBandLimit_);
}

void LteScheduler::schedule()
{
    activeConnectionSet_ = eNbScheduler_->readActiveConnections();

    // obtain the list of cids that can be scheduled on this carrier
    carrierActiveConnectionSet_ = eNbScheduler_->readCarrierActiveConnections(carrierFrequency_);

    // scheduling
    prepareSchedule();
    commitSchedule();
}

void LteScheduler::buildCarrierActiveConnectionSet()
{
    carrierActiveConnectionSet_.clear();

    // put in the activeConnectionSet only connections that are active
    // and whose UE is enabled to use this carrier

    if (binder_ == NULL)
        binder_ = getBinder();

    const UeSet& carrierUeSet = binder_->getCarrierUeSet(carrierFrequency_);
    ActiveSet::iterator it = activeConnectionSet_->begin();
    for (; it != activeConnectionSet_->end(); ++it)
    {
        if (carrierUeSet.find(MacCidToNodeId(*it)) != carrierUeSet.end())
            carrierActiveConnectionSet_.insert(*it);
    }
}
