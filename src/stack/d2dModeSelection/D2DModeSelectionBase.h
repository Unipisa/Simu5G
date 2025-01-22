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

#ifndef LTE_D2DMODESELECTIONBASE_H_
#define LTE_D2DMODESELECTIONBASE_H_

#include <inet/common/ModuleRefByPar.h>

#include "stack/mac/LteMacEnbD2D.h"

namespace simu5g {

using namespace omnetpp;

//
// D2DModeSelectionBase
// Base class for D2D Mode Selection modules
// To add a new policy for mode selection, extend this class and redefine pure virtual methods
//
class D2DModeSelectionBase : public cSimpleModule
{

  protected:

    typedef std::pair<MacNodeId, MacNodeId> FlowId;
    struct FlowModeInfo {
        FlowId flow;
        LteD2DMode oldMode;
        LteD2DMode newMode;
    };
    typedef std::list<FlowModeInfo> SwitchList;
    SwitchList switchList_;  // a list of pairs of nodeIds, where the first node represents the transmitter
                             // of the flow, whereas the second node represents the receiver

    // for each D2D-capable UE, store the list of possible D2D peers and the corresponding communication mode (IM or DM)
    std::map<MacNodeId, std::map<MacNodeId, LteD2DMode>> *peeringModeMap_;

    // reference to the MAC layer
    inet::ModuleRefByPar<LteMacEnbD2D> mac_;

    // reference to the binder
    inet::ModuleRefByPar<Binder> binder_;

    // period between two selection instances
    double modeSelectionPeriod_;

    // Self message
    cMessage *modeSelectionTick_ = nullptr;

    // run the mode selection algorithm. To be implemented by derived classes
    // it must build a switch list (see above)
    virtual void doModeSelection() {}

    // for each pair of UEs in the switch list, send the notification to do the
    // switch to the transmitter UE
    void sendModeSwitchNotifications();

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

