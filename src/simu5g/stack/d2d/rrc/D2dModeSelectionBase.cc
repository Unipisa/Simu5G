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

#include "simu5g/stack/d2d/rrc/D2dModeSelectionBase.h"

namespace simu5g {

Define_Module(D2dModeSelectionBase);

using namespace inet;
using namespace omnetpp;

void D2dModeSelectionBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        // get reference to mac layer (must be a D2D-capable eNB MAC)
        mac_.reference(this, "macModule", true);
        d2dMac_ = check_and_cast<ID2dMacEnb *>(mac_.get());

        // get reference to the binder
        binder_.reference(this, "binderModule", true);
        connectionControl_.reference(this, "connectionControlModule", true);
        d2dBinder_.reference(this, "d2dBinderModule", true);

        // get mode selection period
        modeSelectionPeriod_ = par("modeSelectionPeriod").doubleValue();
        if (modeSelectionPeriod_ < TTI)
            modeSelectionPeriod_ = TTI;

        // Start mode selection tick
        modeSelectionTick_ = new cMessage("modeSelectionTick");
        modeSelectionTick_->setSchedulingPriority(1);  // do mode selection after the (possible) reception of data from the upper layers
        scheduleAt(NOW + 0.05, modeSelectionTick_);
    }
}

void D2dModeSelectionBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == modeSelectionTick_) {
            // run mode selection algorithm
            doModeSelection();

            // send switch notifications to selected flows
            sendModeSwitchNotifications();

            scheduleAt(NOW + modeSelectionPeriod_, msg);
        }
        else
            throw cRuntimeError("D2dModeSelectionBase::handleMessage - Unrecognized self message %s", msg->getName());
    }
    else {
        delete msg;
    }
}

void D2dModeSelectionBase::doModeSwitchAtHandover(MacNodeId nodeId, bool handoverCompleted)
{
    EV << NOW << " D2dModeSelectionBase::doModeSwitchAtHandover - Force mode switching for UE " << nodeId << " (handover)" << endl;

    LteD2DMode newMode;
    if (handoverCompleted)
        newMode = DM;
    else
        newMode = IM;

    switchList_.clear();
    for (const auto& [srcId, peerModes] : d2dBinder_->getD2DPeeringModeMap()) {
        for (const auto& [dstId, oldMode] : peerModes) {
            if (srcId != nodeId && dstId != nodeId)
                continue;

            if (oldMode == newMode)
                continue;

            // check if the two peers are under the same cell
            // if not, do not perform the switch
            if (newMode == DM && binder_->getServingNodeOrSelf(srcId) != binder_->getServingNodeOrSelf(dstId))
                continue;

            // add this flow to the list of flows to be switched
            FlowId p(srcId, dstId);
            FlowModeInfo info;
            info.flow = p;
            info.oldMode = oldMode;
            info.newMode = newMode;
            switchList_.push_back(info);

            // update peering map
            d2dBinder_->setD2DMode(srcId, dstId, newMode);

            EV << NOW << " D2dModeSelectionBase::doModeSwitchAtHandover - Flow: " << srcId << " --> " << dstId << " [" << d2dModeToA(newMode) << "]" << endl;
        }
    }

    // send switching command
    sendModeSwitchNotifications();

    switchList_.clear();
}

void D2dModeSelectionBase::sendModeSwitchNotifications()
{
    for (const auto& switchItem : switchList_) {
        MacNodeId srcId = switchItem.flow.first;
        MacNodeId dstId = switchItem.flow.second;
        LteD2DMode oldMode = switchItem.oldMode;
        LteD2DMode newMode = switchItem.newMode;

        d2dMac_->sendModeSwitchNotification(srcId, dstId, oldMode, newMode);
    }
}

} //namespace
