//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_D2DENBMACHELPER_H_
#define _LTE_D2DENBMACHELPER_H_

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/d2d/mac/ID2dMacEnb.h"   // for the HarqBuffersMirrorD2D / D2DPair typedefs

namespace simu5g {

class LteMacEnb;
class Binder;
class ConflictGraph;

/*
 * Plain helper that holds the D2D-specific state and logic shared by every
 * D2D-capable eNB/gNB MAC (LTE and NR variants). It is owned by value by the
 * MAC module and only uses the public API of LteMacEnb/LteMacBase, so that a
 * MAC module that does NOT derive from LteMacEnbD2D can reuse it.
 *
 * Note: self-messages (the periodic conflict-graph update timer, the mode
 * switch notification) belong to the owning module and therefore stay in the
 * leaf; the leaf's timer/message handlers call into this helper.
 */
class D2dEnbMacHelper
{
  protected:
    // the MAC module owning this helper
    LteMacEnb *mac_;

    /*
     * Stores the mirrored status of H-ARQ buffers for D2D transmissions.
     * The key value of the inner map is the pair <sender,receiver> of the D2D flow.
     */
    std::map<GHz, HarqBuffersMirrorD2D> harqBuffersMirrorD2D_;

    // Conflict Graph builder (nullptr when frequency reuse is disabled)
    ConflictGraph *conflictGraph_ = nullptr;

    // parameters for the conflict graph (needed when frequency reuse is enabled)
    bool reuseD2D_ = false;
    bool reuseD2DMulti_ = false;
    simtime_t conflictGraphUpdatePeriod_;

    // handling of D2D mode switch
    bool msHarqInterrupt_ = false;   // if true, H-ARQ processes of D2D flows are interrupted at mode switch
                                     // otherwise, they are terminated using the old communication mode
    bool msClearRlcBuffer_ = false;  // if true, SDUs stored in the RLC buffer of D2D flows are dropped

  public:
    D2dEnbMacHelper(LteMacEnb *mac) : mac_(mac) {}
    D2dEnbMacHelper(const D2dEnbMacHelper&) = delete;
    D2dEnbMacHelper& operator=(const D2dEnbMacHelper&) = delete;
    ~D2dEnbMacHelper();

    // mirror H-ARQ buffers
    std::map<GHz, HarqBuffersMirrorD2D>& getHarqBuffersMirrorD2DMap() { return harqBuffersMirrorD2D_; }
    HarqBuffersMirrorD2D *getHarqBuffersMirrorD2D(GHz carrierFrequency);
    void deleteHarqBuffersMirrorD2D(MacNodeId txPeer, MacNodeId rxPeer);
    void deleteHarqBuffersMirrorD2D(MacNodeId nodeId);

    // conflict graph / frequency reuse
    bool getReuseD2D() const { return reuseD2D_; }
    void setReuseD2D(bool v) { reuseD2D_ = v; }
    bool getReuseD2DMulti() const { return reuseD2DMulti_; }
    void setReuseD2DMulti(bool v) { reuseD2DMulti_ = v; }
    simtime_t getConflictGraphUpdatePeriod() const { return conflictGraphUpdatePeriod_; }
    void setConflictGraphUpdatePeriod(simtime_t v) { conflictGraphUpdatePeriod_ = v; }
    ConflictGraph *getConflictGraph() const { return conflictGraph_; }

    // build the distance-based conflict graph (uses the current reuse flags)
    void createDistanceBasedConflictGraph(Binder *binder, double threshold,
            double d2dInterferenceRadius, double d2dMultiTxRadius, double d2dMultiInterferenceRadius);
    // recompute the conflict graph (called from the leaf's periodic timer handler)
    void computeConflictGraph();

    // D2D mode-switch handling flags
    bool getMsHarqInterrupt() const { return msHarqInterrupt_; }
    void setMsHarqInterrupt(bool v) { msHarqInterrupt_ = v; }
    bool getMsClearRlcBuffer() const { return msClearRlcBuffer_; }
    void setMsClearRlcBuffer(bool v) { msClearRlcBuffer_ = v; }

    // handle a D2D mode switch notification arriving at the eNB.
    // Note: the caller (leaf handleMessage) owns and deletes pkt.
    void macHandleD2DModeSwitch(cPacket *pkt);
};

} //namespace

#endif
