//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __SIMU5G_LTERLC_H
#define __SIMU5G_LTERLC_H

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

/**
 * LteRlc — extended compound module C++ class.
 * This class adds Binder registration logic for per-DRB instance tracking.
 */
class LteRlc : public omnetpp::cModule
{
  protected:
    int drbId_;           // DRB index
    MacNodeId nodeId_;    // UE's nodeId

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

} // namespace

#endif
