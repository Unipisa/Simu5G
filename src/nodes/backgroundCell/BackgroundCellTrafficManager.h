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

#ifndef BACKGROUNDCELLTRAFFICMANAGER_H_
#define BACKGROUNDCELLTRAFFICMANAGER_H_

#include <inet/common/ModuleRefByPar.h>

#include "common/LteCommon.h"
#include "nodes/backgroundCell/BackgroundCellAmc.h"
#include "stack/backgroundTrafficGenerator/BackgroundTrafficManagerBase.h"

namespace simu5g {

using namespace omnetpp;

//
// BackgroundCellTrafficManager
//
class BackgroundCellTrafficManager : public BackgroundTrafficManagerBase
{
  protected:

    // reference to background scheduler
    inet::ModuleRefByPar<BackgroundScheduler> bgScheduler_;

    // reference to class AMC for this cell
    BackgroundCellAmc *bgAmc_ = nullptr;

  protected:
    double getTtiPeriod() override;
    bool isSetBgTrafficManagerInfoInit() override;
    std::vector<double> getSINR(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower) override;

  public:
    ~BackgroundCellTrafficManager() override;
    void initialize(int stage) override;

    // get the number of RBs
    unsigned int getNumBands() override;

    // returns the bytes per block of the given UE for the given direction
    unsigned int getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir) override {
        throw cRuntimeError("Not implemented");
    }

    // Compute received power for a background UE according to path loss
    double getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus) override;
};

} //namespace

#endif

