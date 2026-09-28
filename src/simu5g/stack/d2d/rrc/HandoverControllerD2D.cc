//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/d2d/rrc/HandoverControllerD2D.h"

#include "simu5g/stack/d2d/binder/D2dBinder.h"
#include "simu5g/stack/d2d/phy/PhyUeD2D.h"
#include "simu5g/stack/d2d/rrc/D2dModeSelectionBase.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/mac/LteMacUe.h"
#include "simu5g/stack/d2d/mac/ID2dAmc.h"
#include "simu5g/stack/ip2nic/HandoverPacketHolderUe.h"
#include "simu5g/stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(HandoverControllerD2D);

void HandoverControllerD2D::handleMessage(cMessage *msg)
{
    // the post-handover D2D mode re-selection trigger is scheduled (and thus
    // consumed) only by this D2D controller
    if (msg->isSelfMessage() && msg->isName("doModeSwitchAtHandover")) {
        onHandoverCompleted();
        delete msg;
        return;
    }
    HandoverController::handleMessage(msg);
}

void HandoverControllerD2D::requestModeSwitchAtServingCell(MacNodeId enbId, bool handoverCompleted)
{
    // ask the cell's mode-selection module to switch the D2D connections of this
    // UE; the eNB may not be D2D-capable, in which case there is nothing to do
    cModule *rrc = binder_->getRrcByNodeId(enbId);
    D2dModeSelectionBase *d2dModeSelection = dynamic_cast<D2dModeSelectionBase *>(rrc->getSubmodule("d2dModeSelection"));
    if (d2dModeSelection != nullptr)
        d2dModeSelection->doModeSwitchAtHandover(nodeId_, handoverCompleted);
    else
        EV_WARN << "HandoverControllerD2D: eNB " << enbId << " is not D2D-capable - no D2D mode switch" << endl;
}

void HandoverControllerD2D::onHandoverStarting()
{
    // D2D-specific: Perform D2D mode switch before handover (only if the PHY is D2D-capable)
    if (dynamic_cast<PhyUeD2D*>(phy_)) {
        if (servingNodeId_ != NODEID_NONE) {
            // Stop active D2D flows (go back to Infrastructure mode)
            // Currently, DM is possible only for UEs served by the same cell

            // Trigger D2D mode switch
            requestModeSwitchAtServingCell(servingNodeId_, false);
        }
    }
}

void HandoverControllerD2D::onHandoverExecuting()
{
    // D2D-specific: detach/attach the D2D direction on the old/new AMC (before common
    // logic), only if the PHY is D2D-capable
    if (dynamic_cast<PhyUeD2D*>(phy_)) {
        if (servingNodeId_ != NODEID_NONE) {
            LteAmc *oldAmc = check_and_cast<LteMacEnb *>(binder_->getMacFromMacNodeId(servingNodeId_))->getAmc();
            // The old serving eNB may not be D2D-capable (its AMC would throw on a D2D direction); skip if so.
            if (dynamic_cast<ID2dAmc *>(oldAmc) != nullptr)
                oldAmc->detachUser(nodeId_, D2D);
            else
                EV_WARN << "HandoverControllerD2D: serving eNB " << servingNodeId_ << " is not D2D-capable - skipping D2D AMC detach" << endl;
        }

        if (candidateServingNodeId_ != NODEID_NONE) {
            LteAmc *newAmc = getAmcModule(candidateServingNodeId_);
            // The new serving eNB may not be D2D-capable (its AMC would throw on a D2D direction); skip if so.
            if (dynamic_cast<ID2dAmc *>(newAmc) != nullptr)
                newAmc->attachUser(nodeId_, D2D);
            else
                EV_WARN << "HandoverControllerD2D: candidate serving eNB " << candidateServingNodeId_ << " is not D2D-capable - skipping D2D AMC attach" << endl;
        }

        // Schedule the post-handover D2D mode re-selection. In the pre-split core this was
        // scheduled near the tail of HandoverController::doHandover(), guarded by the same PHY
        // capability check and by 'servingNodeId_ != NODEID_NONE' evaluated AFTER servingNodeId_
        // had been set to the candidate -- i.e. exactly 'candidateServingNodeId_ != NODEID_NONE'
        // here. doHandover() inserts no other NOW/priority-10 event between this hook and the old
        // site, so the message's FES ordering is unchanged (self-message runs at end of the TTI).
        if (candidateServingNodeId_ != NODEID_NONE) {
            cMessage *msg = new cMessage("doModeSwitchAtHandover");
            msg->setSchedulingPriority(10);
            scheduleAt(NOW, msg);
        }
    }
}

void HandoverControllerD2D::onNodeLeaving()
{
    // A D2D-capable UE also attached itself to the D2D direction (see LteMacUeD2D::initialize),
    // and onHandoverExecuting() detaches it from the old cell on every handover. Departure has
    // to do the same, or the AMC keeps the departed id in its D2D structures and later trips
    // getD2DCapability()'s "is a registered UE" assert when getFeedbackD2D() walks
    // d2dFeedbackHistory_.
    if (dynamic_cast<PhyUeD2D*>(phy_)) {
        LteAmc *amc = getAmcModule(servingNodeId_);
        // The serving eNB may not be D2D-capable (its AMC would throw on a D2D direction); skip if so.
        if (dynamic_cast<ID2dAmc *>(amc) != nullptr)
            amc->detachUser(nodeId_, D2D);
        else
            EV_WARN << "HandoverControllerD2D: serving eNB " << servingNodeId_ << " is not D2D-capable - skipping D2D AMC detach" << endl;
    }
}

void HandoverControllerD2D::onHandoverCompleted()
{
    // Handover completed: ask the new serving cell's mode selection module whether
    // D2D connections of this UE can switch (back) to Direct Mode.
    requestModeSwitchAtServingCell(servingNodeId_, true);
}

void HandoverControllerD2D::bearerReleased(DrbKey bearer)
{
    Enter_Method_Silent("bearerReleased");
    if (getNodeTypeById(bearer.getNodeId()) == NODEB)
        return;   // an infrastructure bearer: its base station releases the id

    // A dual-stack UE may have established the bearer under either of its own ids, so
    // offer it back to both pools -- releasing an id that is not in use there is a no-op
    D2dBinder *d2dBinder = D2dBinder::getInstance(this);
    MacNodeId otherId = hasOtherLeg() ? otherHandoverController_->getNodeId() : NODEID_NONE;
    for (MacNodeId ownId : {nodeId_, otherId}) {
        if (ownId == NODEID_NONE)
            continue;
        auto pair = std::minmax(ownId, bearer.getNodeId());
        if (d2dBinder->sidelinkDrbIdPool({pair.first, pair.second}).erase(bearer.getDrbId()) != 0)
            EV << "HandoverControllerD2D::bearerReleased - DRB " << bearer.getDrbId() << " of the sidelink node pair ("
               << pair.first << ", " << pair.second << ") is free again" << endl;
    }
}

} //namespace
