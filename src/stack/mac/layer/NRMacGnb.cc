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
{   LteMacEnbD2D::initialize(stage);
    if (stage == inet::INITSTAGE_LINK_LAYER)
    {
        qosHandler = check_and_cast<QosHandlerGNB*>(getParentModule()->getSubmodule("qosHandlerGnb"));
        /* Create and initialize NR MAC Uplink scheduler */
        if (enbSchedulerUl_ == NULL)
        {
            enbSchedulerUl_ = new NRSchedulerGnbUl();
            (enbSchedulerUl_->resourceBlocks()) = cellInfo_->getNumBands();
            enbSchedulerUl_->initialize(UL, this);
        }
       //enbSchedulerDl_ = check_and_cast<LteSchedulerEnbDl*>(new LteSchedulerEnbDl());
        //enbSchedulerDl_->initialize(DL, this);
    }

}
void NRMacGnb::handleMessage(cMessage *msg) {

    //std::cout << "NRMacGnbRealistic::handleMessage start at " << simTime().dbl() << std::endl;

    if (strcmp(msg->getName(), "RRC") == 0) {
        cGate *incoming = msg->getArrivalGate();
        if (incoming == up_[IN_GATE]) {
            //from pdcp to mac
            send(msg, gate("lowerLayer$o"));
        } else if (incoming == down_[IN_GATE]) {
            //from mac to pdcp
            send(msg, gate("upperLayer$o"));
        }
    }

    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "flushHarqMsg") == 0) {
            flushHarqBuffers();
            delete msg;
            return;
        }
    }

    LteMacBase::handleMessage(msg);

    //std::cout << "NRMacGnbRealistic::handleMessage end at " << simTime().dbl() << std::endl;
}

