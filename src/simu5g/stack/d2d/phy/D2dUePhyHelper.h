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

#ifndef _LTE_D2DUEPHYHELPER_H_
#define _LTE_D2DUEPHYHELPER_H_

#include <cmath>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class PhyBase;
class LteAirFrame;
class UserControlInfo;

/*
 * Plain helper that holds the D2D-specific PHY state and logic shared by every
 * D2D-capable UE PHY (LTE and NR variants): the D2D Tx power and the capture-
 * effect machinery for D2D multicast reception. It is owned by value by the PHY
 * module and only uses the public API of PhyBase, so that a PHY module that
 * does NOT derive from PhyUeD2D can reuse it.
 *
 * Note: the decoding self-message (d2dDecodingTimer) belongs to the owning
 * module and therefore stays in the leaf; the leaf schedules it and, when it
 * fires, calls decodeStoredFrames() here. Likewise the leaf calls storeAirFrame()
 * from its air-frame handler after it has (re)scheduled the timer.
 */
class D2dUePhyHelper
{
  protected:
    // the PHY module owning this helper
    PhyBase *phy_;

    // D2D Tx power
    double d2dTxPower_ = 0.0;

    // used in multicast D2D to prevent a sendDirect towards out-of-range UEs;
    // the range is given by multicastD2DRange_
    bool multicastD2DRangeCheckEnabled_ = false;
    double multicastD2DRange_ = NAN;

    // Capture effect for D2D multicast communications
    bool d2dMulticastEnableCaptureEffect_ = false;
    double nearestDistance_ = 0.0;
    std::vector<double> bestRsrpVector_;
    double bestRsrpMean_ = 0.0;
    // airframes received in the current TTI; only one will be decoded (capture effect)
    std::vector<LteAirFrame *> d2dReceivedFrames_;

    // pick the frame to be decoded among those received in the current TTI
    LteAirFrame *extractAirFrame();
    // decode the given frame and hand the decapsulated packet to the PHY's upper layer
    void decodeAirFrame(LteAirFrame *frame, UserControlInfo *lteInfo);

  public:
    D2dUePhyHelper(PhyBase *phy) : phy_(phy) {}

    // one-time WATCH registration; call from the UE PHY's initialize()
    void initWatches();

    // D2D Tx power
    double getD2dTxPower() const { return d2dTxPower_; }
    void setD2dTxPower(double v) { d2dTxPower_ = v; }

    // multicast-D2D range check (used by sendMulticast in the owning leaf)
    bool getMulticastD2DRangeCheckEnabled() const { return multicastD2DRangeCheckEnabled_; }
    void setMulticastD2DRangeCheckEnabled(bool v) { multicastD2DRangeCheckEnabled_ = v; }
    double getMulticastD2DRange() const { return multicastD2DRange_; }
    void setMulticastD2DRange(double v) { multicastD2DRange_ = v; }

    // capture-effect enable flag
    bool getMulticastEnableCaptureEffect() const { return d2dMulticastEnableCaptureEffect_; }
    void setMulticastEnableCaptureEffect(bool v) { d2dMulticastEnableCaptureEffect_ = v; }

    // store a received D2D-multicast airframe, applying the capture effect
    void storeAirFrame(LteAirFrame *newFrame);

    // decode the captured airframe (called from the leaf's decoding-timer handler)
    // and clear the current-TTI receive buffer
    void decodeStoredFrames();
};

} //namespace

#endif
