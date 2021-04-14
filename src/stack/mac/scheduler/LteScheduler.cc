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

/**
 * TODO:
 * - rimuovere i commenti dalle funzioni quando saranno implementate nel enb scheduler
 */

using namespace omnetpp;


void LteScheduler::setEnbScheduler(LteSchedulerEnb* eNbScheduler)
{
    eNbScheduler_ = eNbScheduler;
    direction_ = eNbScheduler_->direction_;
    mac_ = eNbScheduler_->mac_;
    initializeGrants();
}

void LteScheduler::setCarrierFrequency(double carrierFrequency)
{
    carrierFrequency_ = carrierFrequency;
    bandLimit_ = mac_->getCellInfo()->getCarrierBandLimit(carrierFrequency_);

    // === DEBUG === //
    EV << "LteScheduler::setCarrierFrequency - Set Band Limit for this carrier: " << endl;
    BandLimitVector::iterator it = bandLimit_->begin();
    for (; it != bandLimit_->end(); ++it)
    {
        EV << " - Band[" << it->band_ << "] limit[";
        for (int cw=0; cw < it->limit_.size(); cw++)
        {
            EV << it->limit_[cw] << ", ";
        }
        EV << "]" << endl;
    }
    // === END DEBUG === //
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
    BandLimitVector tempBandLimit;
    tempBandLimit.clear();
    if (bandLim == NULL)
    {
        // for each band of the band vector provided
        BandLimitVector::iterator it = bandLimit_->begin();
        for (; it != bandLimit_->end(); ++it)
        {
            BandLimit elem;
            // copy the band
            elem.band_ = it->band_;
            elem.limit_ = it->limit_;

            tempBandLimit.push_back(elem);
        }
        bandLim = &tempBandLimit;
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

bool LteScheduler::scheduleRetransmissions()
{
    BandLimitVector tempBandLimit;

    // for each band of the band vector provided
    BandLimitVector::iterator it = bandLimit_->begin();
    for (; it != bandLimit_->end(); ++it)
    {
        BandLimit elem;
        // copy the band
        elem.band_ = it->band_;
        elem.limit_ = it->limit_;

        tempBandLimit.push_back(elem);
    }

    return eNbScheduler_->rtxschedule(carrierFrequency_, &tempBandLimit);
}

void LteScheduler::scheduleRacRequests()
{
    //return (dynamic_cast<LteSchedulerEnbUl*>(eNbScheduler_))->serveRacs();
}

void LteScheduler::requestRacGrant(MacNodeId nodeId)
{
    //return (dynamic_cast<LteSchedulerEnbUl*>(eNbScheduler_))->racGrantEnb(nodeId);
}

void LteScheduler::schedule()
{
    activeConnectionSet_ = eNbScheduler_->readActiveConnections();

    // obtain the list of cids that can be scheduled on this carrier
    buildCarrierActiveConnectionSet();

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

void LteScheduler::initializeGrants()
{
    if (direction_ == DL)
    {
        grantTypeMap_[CONVERSATIONAL] = aToGrantType(mac_->par("grantTypeConversationalDl"));
        grantTypeMap_[STREAMING] = aToGrantType(mac_->par("grantTypeStreamingDl"));
        grantTypeMap_[INTERACTIVE] = aToGrantType(mac_->par("grantTypeInteractiveDl"));
        grantTypeMap_[BACKGROUND] = aToGrantType(mac_->par("grantTypeBackgroundDl"));

        grantSizeMap_[CONVERSATIONAL] = mac_->par("grantSizeConversationalDl");
        grantSizeMap_[STREAMING] = mac_->par("grantSizeStreamingDl");
        grantSizeMap_[INTERACTIVE] = mac_->par("grantSizeInteractiveDl");
        grantSizeMap_[BACKGROUND] = mac_->par("grantSizeBackgroundDl");
    }
    else if (direction_ == UL)
    {
        grantTypeMap_[CONVERSATIONAL] = aToGrantType(mac_->par("grantTypeConversationalUl"));
        grantTypeMap_[STREAMING] = aToGrantType(mac_->par("grantTypeStreamingUl"));
        grantTypeMap_[INTERACTIVE] = aToGrantType(mac_->par("grantTypeInteractiveUl"));
        grantTypeMap_[BACKGROUND] = aToGrantType(mac_->par("grantTypeBackgroundUl"));

        grantSizeMap_[CONVERSATIONAL] = mac_->par("grantSizeConversationalUl");
        grantSizeMap_[STREAMING] = mac_->par("grantSizeStreamingUl");
        grantSizeMap_[INTERACTIVE] = mac_->par("grantSizeInteractiveUl");
        grantSizeMap_[BACKGROUND] = mac_->par("grantSizeBackgroundUl");
    }
    else
    {
        throw cRuntimeError("Unknown direction %d", direction_);
    }
}
