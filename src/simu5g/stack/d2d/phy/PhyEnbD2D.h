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

#ifndef _PHYENBD2D_H_
#define _PHYENBD2D_H_

#include "simu5g/stack/phy/PhyEnb.h"

namespace simu5g {

using namespace omnetpp;

class D2dBinder;

/**
 * D2D-capable eNB PHY. The generic receive/transmit/feedback paths live in
 * PhyEnb/PhyBase; this class only fills their seams:
 * - appendExtraFeedback(): per-peer D2D CQI reporting (if enabled)
 * - airFrameNameFor()/airFramePriorityFor(): the eNB/gNB is the sole sender
 *   of D2D mode-switch notifications; such frames get a distinctive name and
 *   an elevated (-1) scheduling priority so they are processed ahead of
 *   ordinary air-frames starting/ending at the same instant.
 */
class PhyEnbD2D : public PhyEnb
{
    bool enableD2DCqiReporting_;

    // holder of the global D2D state
    inet::ModuleRefByPar<D2dBinder> d2dBinder_;

  protected:
    void initialize(int stage) override;

    void appendExtraFeedback(inet::Ptr<LteFeedbackPkt>& header, UserControlInfo *lteinfo, LteAirFrame *frame, ChannelModelBase *channelModel) override;

    const char *airFrameNameFor(const UserControlInfo *info) override
    {
        if (info->getFrameType() == D2DMODESWITCHPKT)
            return "d2dModeSwitch";
        return PhyEnb::airFrameNameFor(info);
    }

    short airFramePriorityFor(const UserControlInfo *info) override
    {
        if (info->getFrameType() == D2DMODESWITCHPKT)
            return -1;
        return PhyEnb::airFramePriorityFor(info);
    }
};

} //namespace

#endif /* _PHYENBD2D_H_ */
