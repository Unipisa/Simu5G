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

#ifndef _LTE_D2DAMCHELPER_H_
#define _LTE_D2DAMCHELPER_H_

#include "simu5g/stack/mac/amc/LteAmc.h"

namespace simu5g {

class D2dBinder;

/*
 * Plain helper that holds the D2D-specific AMC state and the heavy D2D logic
 * shared by every D2D-capable AMC (LteAmcD2D and NrAmcD2D). It is owned by
 * value by the D2D AMC module and uses only the public API of LteAmc, so that
 * an AMC that does NOT derive from LteAmcD2D (i.e. NrAmcD2D) can reuse it.
 *
 * The D2D feedback / transmission-parameter structures live here, moved out of
 * the core LteAmc, which keeps only the DL/UL structures and routes any D2D
 * direction to a protected virtual seam; the D2D leaf implements that seam by
 * delegating to this helper.
 */
class D2dAmcHelper
{
  protected:
    // the D2D AMC owning this helper
    LteAmc *amc_;

    // D2D-specific state (moved out of core LteAmc)
    double mcsScaleD2D_ = 0;
    ConnectedUesMap d2dConnectedUe_;
    std::map<MacNodeId, unsigned int> d2dNodeIndex_;
    std::vector<MacNodeId> d2dRevNodeIndex_;
    std::map<GHz, std::vector<UserTxParams>> d2dTxParams_;
    std::map<GHz, std::map<MacNodeId, History_>> d2dFeedbackHistory_;
    unsigned int fbhbCapacityD2D_ = 0;

    // confidence-function bounds, cached from the owning MAC at initD2D()
    simtime_t lb_;
    simtime_t ub_;

    // holder of the global D2D state, resolved (find-or-create) on first D2D use
    D2dBinder *d2dBinder_ = nullptr;

  public:
    D2dAmcHelper(LteAmc *amc) : amc_(amc) {}

    // one-time D2D structure setup; call from the D2D AMC initialize() after LteAmc::initialize()
    void initD2D();

    // feedback management (ID2dAmc surface)
    void pushFeedbackD2D(MacNodeId id, LteFeedback fb, MacNodeId peerId, GHz carrierFrequency);
    const LteSummaryFeedback& getFeedbackD2D(MacNodeId id, Remote antenna, TxMode txMode, MacNodeId peerId, GHz carrierFrequency);
    // the NOSIGNALCQI summary handed out while a carrier has no D2D feedback yet
    const LteSummaryFeedback& noFeedbackD2D(std::map<MacNodeId, History_>& carrierHistory, TxMode txMode);

    // D2D branch of the base tx-param routing seams
    bool existTxParamsD2D(MacNodeId id, GHz carrierFrequency);
    const UserTxParams& setTxParamsD2D(MacNodeId id, UserTxParams& info, GHz carrierFrequency);
    const UserTxParams& getTxParamsD2D(MacNodeId id, GHz carrierFrequency);
    void printTxParamsD2D(GHz carrierFrequency);

    // D2D MCS rescaling
    void rescaleD2D(double rePerRb);

    /**
     * Forget a node that has left the simulation.
     *
     * Unlike detachUserD2D(), this must work for a node that was never attached to *this*
     * AMC: a UE served by another cell still shows up here as a D2D feedback peer, so its id
     * outlives it in d2dFeedbackHistory_ and any later walk of that map hands the id to
     * D2dBinder::getD2DCapability(), which asserts that it is a registered UE. Driven by the
     * Binder's node-unregistered notification, which reaches every D2D AMC in the network.
     */
    void purgeDepartedNode(MacNodeId nodeId);

    // handover support
    void detachUserD2D(MacNodeId nodeId);
    void attachUserD2D(MacNodeId nodeId);
    void testUeD2D(MacNodeId nodeId);
};

} //namespace

#endif
