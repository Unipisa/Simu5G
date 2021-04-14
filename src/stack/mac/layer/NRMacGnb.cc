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

#include "stack/mac/layer/NRMacGnb.h"
#include "stack/mac/scheduler/NRSchedulerGnbUl.h"

Define_Module(NRMacGnb);

NRMacGnb::NRMacGnb() :  LteMacEnbD2D()
{
    nodeType_ = GNODEB;
}

NRMacGnb::~NRMacGnb()
{
}

void NRMacGnb::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LINK_LAYER)
    {
        /* Create and initialize NR MAC Uplink scheduler */
        if (enbSchedulerUl_ == NULL)
        {
            enbSchedulerUl_ = new NRSchedulerGnbUl();
            enbSchedulerUl_->initialize(UL, this);
        }
    }
    LteMacEnbD2D::initialize(stage);
}
