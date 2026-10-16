//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/nodes/ExtCellAllocator.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(ExtCellAllocator);

void ExtCellAllocator::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        carrierFrequency_ = GHz(par("carrierFrequency"));
        numBands_ = par("numBands");

        binder_.reference(this, "binderModule", true);
        nodeId_ = binder_->allocatePhantomBaseStationId();
        radio_.reference(this, "radioModule", true);
        radio_->addToMedium(nodeId_);

        // initialize band status structures
        bandStatus_.resize(numBands_, 0);
        prevBandStatus_.resize(numBands_, 0);

        // get allocation type
        std::string allocationType = par("bandAllocationType").stdstringValue();
        if (allocationType == "CONTIGUOUS_ALLOC") {
            allocationType_ = CONTIGUOUS_ALLOC;
        }
        else if (allocationType == "RANDOM_ALLOC") {
            allocationType_ = RANDOM_ALLOC;
        }
        else if (allocationType == "FULL_ALLOC") {
            allocationType_ = FULL_ALLOC;
        }
        else
            throw cRuntimeError("Unrecognized bandAllocationType: '%s'", allocationType.c_str());

        // get the allocation parameters
        if (allocationType_ == FULL_ALLOC) {
            // Mark all RBs as occupied. assign(), not resize(): the vectors
            // already have numBands_ elements from above, and resize() only
            // initializes the elements it adds. Nothing updates them later,
            // because FULL_ALLOC schedules no TTI tick.
            bandStatus_.assign(numBands_, 1);
            prevBandStatus_.assign(numBands_, 1);
        }
        else {
            // get the band utilization
            double bandUtilization = par("bandUtilization");
            setBandUtilization(bandUtilization);

            if (allocationType_ == CONTIGUOUS_ALLOC) {
                // get the starting offset
                startingOffset_ = par("startingOffset");
            }

            // TODO: if extCell-interference is disabled, do not send selfMessages
            // Start TTI tick
            ttiTick_ = new cMessage("ttiTick_");
            ttiTick_->setSchedulingPriority(1);        // TTI TICK after other messages
            scheduleAt(NOW + TTI, ttiTick_);
        }

        WATCH(id_);
        WATCH(bandStatus_);
        WATCH(prevBandStatus_);
    }
    if (stage == inet::INITSTAGE_SINGLE_MOBILITY && allocationType_ == FULL_ALLOC) {
        // on the radio medium, a transmission on every band that never ends,
        // from where the radio is: the mobility, declared before the
        // allocator, has its position from this stage on
        registerTransmission(SimTime::getMaxTime() - NOW);
    }
    if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        // add this cell to the binder
        id_ = binder_->addExtCell(this, carrierFrequency_);
    }
}

void ExtCellAllocator::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        updateBandStatus();
        registerTransmission(TTI);

        scheduleAt(NOW + TTI, msg);
        return;
    }
}

void ExtCellAllocator::updateBandStatus()
{
    EV << "----- EXT CELL ALLOCATION UPDATE -----" << std::endl;

    resetBandStatus();

    if (allocationType_ == RANDOM_ALLOC) {
        EV << " ExtCell::updateBandStatus() - generating new random allocation for extCell " << id_ << " (carrier " << carrierFrequency_ << ")" << std::endl;

        // allocates each band with probability equal to bandUtilization_
        for (unsigned int band = 0; band < numBands_; ++band) {
            int occ = bernoulli(bandUtilization_);
            bandStatus_[band] = occ;
        }
    }
    else {  // CONTIGUOUS ALLOC
        EV << " ExtCell::updateBandStatus() - generating new contiguous allocation for extCell " << id_ << " (carrier " << carrierFrequency_ << ")" << std::endl;

        // get the number of bands to be allocated
        int toAlloc = ceil((double)numBands_ * bandUtilization_);
        int band = startingOffset_;
        int prev = band;
        for ( ; (band != startingOffset_ || band == prev) && toAlloc > 0; prev = band, band = (band + 1) % numBands_) {
            bandStatus_[band] = 1;
            toAlloc--;
        }
    }

    EV << "----- END EXT CELL ALLOCATION UPDATE -----" << std::endl;
}

void ExtCellAllocator::registerTransmission(simtime_t duration)
{
    RbMap blocks;
    for (unsigned int band = 0; band < numBands_; ++band)
        if (bandStatus_[band] != 0)
            blocks[MACRO][band] = 1;
    if (blocks.empty())
        return;

    // the cell is no node: its phantom radio sends the transmission
    radio_->transmit(NODEID_NONE, nodeId_, DL, carrierFrequency_, blocks, duration, this);
}

void ExtCellAllocator::resetBandStatus()
{
    prevBandStatus_ = bandStatus_;

    bandStatus_.clear();
    bandStatus_.resize(numBands_, 0);
}

void ExtCellAllocator::setBandUtilization(double bandUtilization)
{
    if (bandUtilization < 0)
        bandUtilization = 0;
    else if (bandUtilization > 1)
        bandUtilization = 1;

    bandUtilization_ = bandUtilization;
}

} //namespace
