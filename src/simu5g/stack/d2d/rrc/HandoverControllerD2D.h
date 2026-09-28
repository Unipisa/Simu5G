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

#ifndef _SIMU5G_HANDOVERCONTROLLERD2D_H_
#define _SIMU5G_HANDOVERCONTROLLERD2D_H_

#include "simu5g/stack/rrc/HandoverController.h"

namespace simu5g {

using namespace omnetpp;

//
// D2D-capable variant of the HandoverController. It fills in the handover
// lifecycle hooks with the D2D behavior (mode switch to infrastructure before
// handover, D2D AMC detach/attach while executing, D2D mode re-selection once
// completed). See HandoverControllerD2D.ned.
//
class HandoverControllerD2D : public HandoverController
{
  protected:
    /// Switch active D2D flows back to Infrastructure mode before the handover.
    /// consumes the post-handover D2D mode re-selection trigger this
    /// controller schedules; everything else goes to the base dispatcher
    void handleMessage(cMessage *msg) override;

    /// common D2D mode-switch request towards a cell's mode-selection module
    void requestModeSwitchAtServingCell(MacNodeId enbId, bool handoverCompleted);

    void onHandoverStarting() override;
    /// Detach/attach the D2D AMC direction on the old/new cell, and schedule the
    /// post-handover D2D mode re-selection.
    void onHandoverExecuting() override;
    /// Ask the new serving cell's mode selection whether D2D flows can go (back) to Direct Mode.
    void onHandoverCompleted() override;
    /// Detach the D2D AMC direction on the serving cell when the UE leaves the simulation.
    void onNodeLeaving() override;

  public:
    /// A sidelink bearer's identity (the peer is a UE or a multicast group) returns to
    /// the network-wide sidelink pool of the pair, kept in the D2dBinder; an
    /// infrastructure bearer's is its base station's to release.
    void bearerReleased(DrbKey bearer) override;
};

} //namespace

#endif
