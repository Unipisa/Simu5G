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

#include "nodes/ExtCell.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(ExtCell);

void ExtCell::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        // get coordinates
        position_.x = par("position_x");
        position_.y = par("position_y");
        position_.z = par("position_z");
        if (getEnvir()->isGUI()) {
            getDisplayString().setTagArg("p", 0, (long)position_.x);
            getDisplayString().setTagArg("p", 1, (long)position_.y);
        }

        txPower_ = par("txPower");

        // set TX direction
        std::string txDir = par("txDirection");
        txDirection_ = static_cast<TxDirectionType>(cEnum::get("simu5g::TxDirectionType")->lookup(txDir.c_str()));
        switch (txDirection_) {
            case OMNI: txAngle_ = 0.0;
                break;
            case ANISOTROPIC: txAngle_ = par("txAngle");
                break;
            default: throw cRuntimeError("unknown txDirection: '%s'", txDir.c_str());
        }

        carrierFrequency_ = par("carrierFrequency").doubleValue();
        numBands_ = par("numBands");
    }
    if (stage == inet::INITSTAGE_LOCAL + 1) {
        binder_.reference(this, "binderModule", true);

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
            // mark all RBs as occupied
            bandStatus_.resize(numBands_, 1);
            prevBandStatus_.resize(numBands_, 1);
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

        // add this cell to the binder
        id_ = binder_->addExtCell(this, carrierFrequency_);
    }
}

void ExtCell::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        updateBandStatus();

        scheduleAt(NOW + TTI, msg);
        return;
    }
}

void ExtCell::updateBandStatus()
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

void ExtCell::resetBandStatus()
{
    prevBandStatus_ = bandStatus_;

    bandStatus_.clear();
    bandStatus_.resize(numBands_, 0);
}

void ExtCell::setBandUtilization(double bandUtilization)
{
    if (bandUtilization < 0)
        bandUtilization = 0;
    else if (bandUtilization > 1)
        bandUtilization = 1;

    bandUtilization_ = bandUtilization;
}

} //namespace

