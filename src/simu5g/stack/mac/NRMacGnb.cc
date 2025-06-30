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
#include "stack/mac/NRMacGnb.h"
#include "stack/mac/scheduler/NRSchedulerGnbUl.h"

namespace simu5g {

Define_Module(NRMacGnb);

NRMacGnb::NRMacGnb() : LteMacEnbD2D()
{
    nodeType_ = GNODEB;
}


void NRMacGnb::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LINK_LAYER) {
        // Create and initialize NR MAC Uplink scheduler
        if (enbSchedulerUl_ == nullptr) {
            enbSchedulerUl_ = new NRSchedulerGnbUl();
            enbSchedulerUl_->resourceBlocks() = cellInfo_->getNumBands();
            enbSchedulerUl_->initialize(UL, this, binder_);
        }
    }
    LteMacEnbD2D::initialize(stage);
}

} //namespace

