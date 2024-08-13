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

#ifndef BACKGROUNDTRAFFICMANAGER_H_
#define BACKGROUNDTRAFFICMANAGER_H_

#include <inet/common/ModuleRefByPar.h>

#include "common/LteCommon.h"
#include "common/blerCurves/PhyPisaData.h"
#include "stack/backgroundTrafficGenerator/BackgroundTrafficManagerBase.h"
#include "stack/backgroundTrafficGenerator/generators/TrafficGeneratorBase.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/phy/layer/LtePhyEnb.h"

namespace simu5g {

using namespace omnetpp;

class TrafficGeneratorBase;
class LteMacEnb;
class LteChannelModel;

//
// BackgroundTrafficManager
//
class BackgroundTrafficManager : public BackgroundTrafficManagerBase
{
  protected:
    // references to the MAC and PHY layer of the e/gNodeB
    inet::ModuleRefByPar<LteMacEnb> mac_;

    // reference to phy module
    inet::ModuleRefByPar<LtePhyEnb> phy_;

    // reference to the channel model for the given carrier
    LteChannelModel *channelModel_ = nullptr;

  protected:
    virtual void initialize(int stage);

    virtual double getTtiPeriod();
    virtual bool isSetBgTrafficManagerInfoInit();
    virtual std::vector<double> getSINR(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower);

  public:
    BackgroundTrafficManager();

    // get the number of RBs
    virtual unsigned int getNumBands();

    // returns the bytes per block of the given UE in the given direction
    virtual unsigned int getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir);

    // Compute received power for a background UE according to path loss
    virtual double getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus);
};

} //namespace

#endif

