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

#ifndef LTE_D2DMODESELECTIONBASE_H_
#define LTE_D2DMODESELECTIONBASE_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/d2d/mac/ID2dMacEnb.h"
#include "simu5g/stack/d2d/binder/D2dBinder.h"
#include "simu5g/stack/rrc/ConnectionControlEnb.h"

namespace simu5g {

using namespace omnetpp;

//
// D2dModeSelectionBase
// Base class for D2D Mode Selection modules
// To add a new policy for mode selection, extend this class and redefine pure virtual methods
//
class D2dModeSelectionBase : public cSimpleModule
{

  protected:

    typedef std::pair<MacNodeId, MacNodeId> FlowId;
    struct FlowModeInfo {
        FlowId flow;
        LteD2DMode oldMode;
        LteD2DMode newMode;

        friend std::ostream& operator<<(std::ostream& os, const FlowModeInfo& f)
        {
            os << "flow=(" << f.flow.first << "->" << f.flow.second << ") old=" << f.oldMode << " new=" << f.newMode;
            return os;
        }
    };
    typedef std::list<FlowModeInfo> SwitchList;
    SwitchList switchList_;  // a list of pairs of nodeIds, where the first node represents the transmitter
                             // of the flow, whereas the second node represents the receiver

    // holder of the global D2D state (incl. the per-UE peering/mode map)
    inet::ModuleRefByPar<D2dBinder> d2dBinder_;

    // reference to the MAC layer
    inet::ModuleRefByPar<LteMacEnb> mac_;

    // D2D view of the same MAC module, resolved at initialization
    ID2dMacEnb *d2dMac_ = nullptr;

    // reference to the binder
    inet::ModuleRefByPar<Binder> binder_;

    // the node's control-plane entry point, which knows the UEs handing over
    inet::ModuleRefByPar<ConnectionControlEnb> connectionControl_;

    // period between two selection instances
    double modeSelectionPeriod_;

    // Self message
    cMessage *modeSelectionTick_ = nullptr;

    // run the mode selection algorithm. To be implemented by derived classes
    // it must build a switch list (see above)
    virtual void doModeSelection() {}

    // for each pair of UEs in the switch list, send the notification to do the
    // switch to the transmitter UE
    virtual void sendModeSwitchNotifications();

  public:

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

    // this method triggers possible switches after handover
    // if handoverCompleted is false, move all the connections for nodeId to IM
    // if handoverCompleted is true, move the connections for nodeId to DM only if the endpoint is under the same cell
    //
    // NOTE: re-implement this method in derived classes
    virtual void doModeSwitchAtHandover(MacNodeId nodeId, bool handoverCompleted);
};

} //namespace

#endif /* LTE_D2DMODESELECTIONBASE_H_ */

