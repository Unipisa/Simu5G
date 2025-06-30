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

#include "stack/d2dModeSelection/D2DModeSelectionBestCqi.h"

namespace simu5g {

Define_Module(D2DModeSelectionBestCqi);

using namespace omnetpp;
using namespace inet;

void D2DModeSelectionBestCqi::initialize(int stage)
{
    D2DModeSelectionBase::initialize(stage);
}

void D2DModeSelectionBestCqi::doModeSelection()
{
    EV << NOW << " D2DModeSelectionBestCqi::doModeSelection - Running Mode Selection algorithm..." << endl;

    // TODO check if correct
    double primaryCarrierFrequency = mac_->getCellInfo()->getCarriers()->front();

    switchList_.clear();
    for (auto& it : *peeringModeMap_) {
        MacNodeId srcId = it.first;

        // consider only UEs within this cell
        if (binder_->getNextHop(srcId) != mac_->getMacCellId())
            continue;

        for (auto& jt : it.second) {
            MacNodeId dstId = jt.first;   // since the D2D CQI is the same for all D2D connections,
                                           // the mode will be the same for all destinations

            // consider only UEs within this cell
            if (binder_->getNextHop(dstId) != mac_->getMacCellId())
                continue;

            // skip UEs that are performing handover
            if (binder_->hasUeHandoverTriggered(dstId) || binder_->hasUeHandoverTriggered(srcId))
                continue;

            LteD2DMode oldMode = jt.second;

            // Compute the achievable bits on a single RB for UL direction
            // Note that this operation takes into account the CQI returned by the AMC Pilot (by default, it
            // is the minimum CQI over all RBs)
            unsigned int bitsUl = mac_->getAmc()->computeBitsOnNRbs(srcId, 0, 0, 1, UL, primaryCarrierFrequency);
            unsigned int bitsD2D = mac_->getAmc()->computeBitsOnNRbs(srcId, 0, 0, 1, D2D, primaryCarrierFrequency);

            EV << NOW << " D2DModeSelectionBestCqi::doModeSelection - bitsUl[" << bitsUl << "] bitsD2D[" << bitsD2D << "]" << endl;

            // compare the bits in the two modes and select the best one
            LteD2DMode newMode = (bitsUl > bitsD2D) ? IM : DM;

            if (newMode != oldMode) {
                // add this flow to the list of flows to be switched
                FlowId p(srcId, dstId);
                FlowModeInfo info;
                info.flow = p;
                info.oldMode = oldMode;
                info.newMode = newMode;
                switchList_.push_back(info);

                // update peering map
                jt.second = newMode;

                EV << NOW << " D2DModeSelectionBestCqi::doModeSelection - Flow: " << srcId << " --> " << dstId << " [" << d2dModeToA(newMode) << "]" << endl;
            }
        }
    }
}

void D2DModeSelectionBestCqi::doModeSwitchAtHandover(MacNodeId nodeId, bool handoverCompleted)
{
    // with this MS algorithm, connections of nodeId will return to DM after handover only
    // if the algorithm triggers the switch at the next period. Thus, it is not necessary to
    // force the switch here.
    if (handoverCompleted)
        return;

    D2DModeSelectionBase::doModeSwitchAtHandover(nodeId, handoverCompleted);
}

} //namespace

