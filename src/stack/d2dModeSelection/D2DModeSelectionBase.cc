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

#include "stack/d2dModeSelection/D2DModeSelectionBase.h"
#include "stack/mac/layer/LteMacEnbD2D.h"

Define_Module(D2DModeSelectionBase);

using namespace inet;
using namespace omnetpp;

void D2DModeSelectionBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        switchList_.clear();

        // get reference to mac layer
        mac_ = check_and_cast<LteMacEnb*>(getParentModule()->getSubmodule("mac"));

        // get reference to the binder
        binder_ = getBinder();

        // get mode selection period
        modeSelectionPeriod_ = par("modeSelectionPeriod").doubleValue();
        if (modeSelectionPeriod_ < TTI)
            modeSelectionPeriod_ = TTI;

        // get the reference to the peering map in the binder
        peeringModeMap_ = binder_->getD2DPeeringMap();

        // Start mode selection tick
        modeSelectionTick_ = new cMessage("modeSelectionTick");
        modeSelectionTick_->setSchedulingPriority(1);  // do mode selection after the (possible) reception of data from the upper layers
        scheduleAt(NOW + 0.05, modeSelectionTick_);
    }
}

void D2DModeSelectionBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (strcmp(msg->getName(),"modeSelectionTick") == 0)
        {
            // run mode selection algorithm
            doModeSelection();

            // send switch notifications to selected flows
            sendModeSwitchNotifications();

            scheduleAt(NOW+modeSelectionPeriod_, msg);
        }
        else
            throw cRuntimeError("D2DModeSelectionBase::handleMessage - Unrecognized self message %s", msg->getName());
    }
    else
    {
         delete msg;
    }
}

void D2DModeSelectionBase::doModeSwitchAtHandover(MacNodeId nodeId, bool handoverCompleted)
{
    EV << NOW << " D2DModeSelectionBase::doModeSwitchAtHandover - Force mode switching for UE " << nodeId << " (handover)" << endl;

    LteD2DMode newMode;
    if (handoverCompleted)
        newMode = DM;
    else
        newMode = IM;

    switchList_.clear();
    std::map<MacNodeId, std::map<MacNodeId, LteD2DMode> >::iterator it = peeringModeMap_->begin();
    for (; it != peeringModeMap_->end(); ++it)
    {
        MacNodeId srcId = it->first;
        std::map<MacNodeId, LteD2DMode>::iterator jt = it->second.begin();
        for (; jt != it->second.end(); ++jt)
        {
            MacNodeId dstId = jt->first;
            if (srcId != nodeId && dstId != nodeId)
                continue;

            LteD2DMode oldMode = jt->second;
            if (oldMode == newMode)
                continue;

            // check if the two peers are under the same cell
            // if not, do not perform the switch
            if (newMode == DM && binder_->getNextHop(srcId) != binder_->getNextHop(dstId))
                continue;

            // add this flow to the list of flows to be switched
            FlowId p(srcId, dstId);
            FlowModeInfo info;
            info.flow = p;
            info.oldMode = oldMode;
            info.newMode = newMode;
            switchList_.push_back(info);

            // update peering map
            jt->second = newMode;

            EV << NOW << " D2DModeSelectionBase::doModeSwitchAtHandover - Flow: " << srcId << " --> " << dstId << " [" << d2dModeToA(newMode) << "]" << endl;
        }
    }

    // send switching command
    sendModeSwitchNotifications();

    switchList_.clear();
}


void D2DModeSelectionBase::sendModeSwitchNotifications()
{
    SwitchList::iterator it = switchList_.begin();
    for (; it != switchList_.end(); ++it)
    {
        MacNodeId srcId = it->flow.first;
        MacNodeId dstId = it->flow.second;
        LteD2DMode oldMode = it->oldMode;
        LteD2DMode newMode = it->newMode;

        check_and_cast<LteMacEnbD2D*>(mac_)->sendModeSwitchNotification(srcId, dstId, oldMode, newMode);
    }
}
