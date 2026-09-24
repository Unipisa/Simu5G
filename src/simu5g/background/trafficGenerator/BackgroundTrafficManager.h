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

#ifndef BACKGROUNDTRAFFICMANAGER_H_
#define BACKGROUNDTRAFFICMANAGER_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/blerCurves/PhyPisaData.h"
#include "simu5g/background/trafficGenerator/BackgroundTrafficManagerBase.h"
#include "simu5g/background/trafficGenerator/generators/TrafficGeneratorBase.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/phy/PhyEnb.h"

namespace simu5g {

using namespace omnetpp;

class TrafficGeneratorBase;
class LteMacEnb;
class ChannelModelBase;

//
// BackgroundTrafficManager
//
class BackgroundTrafficManager : public BackgroundTrafficManagerBase
{
  protected:
    // references to the MAC and PHY layer of the e/gNodeB
    inet::ModuleRefByPar<LteMacEnb> mac_;

    // reference to phy module
    inet::ModuleRefByPar<PhyEnb> phy_;

    // reference to the channel model for the given carrier
    ChannelModelBase *channelModel_ = nullptr;

  protected:
    void initialize(int stage) override;

    double getTtiPeriod() override;
    bool isSetBgTrafficManagerInfoInit() override;
    std::vector<double> getSINR(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower) override;

  public:

    // get the number of RBs
    unsigned int getNumBands() override;

    // returns the bytes per block of the given UE in the given direction
    unsigned int getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir) override;
};

} //namespace

#endif

