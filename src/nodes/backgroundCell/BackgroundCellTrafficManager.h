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

#include "common/LteCommon.h"
#include "stack/backgroundTrafficGenerator/generators/TrafficGeneratorBase.h"

using namespace omnetpp;

class TrafficGeneratorBase;
class LteMacEnb;
class LteChannelModel;
class BackgroundCellAmc;

//
// BackgroundCellTrafficManager
//
class BackgroundCellTrafficManager : public BackgroundTrafficManager
{
  protected:

    // reference to background scheduler
    BackgroundScheduler* bgScheduler_;

    // reference to class AMC for this cell
    BackgroundCellAmc* bgAmc_;

    static double nrCqiTable[16];
    double getCqiFromTable(double snr);

  public:
    BackgroundCellTrafficManager();
    virtual ~BackgroundCellTrafficManager();
    virtual void initialize(int stage);

    // get the number of RBs
    virtual unsigned int getNumBands();

    // returns the CQI based on the given position and power
    virtual Cqi computeCqi(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower = 0.0);

    // returns the bytes per block of the given UE for in the given direction
    virtual  unsigned int getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir);

    // signal that the RAC for the given UE has been handled
    virtual void racHandled(MacNodeId bgUeId);

    // Compute received power for a background UE according to pathloss
    virtual double getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus);
};

#endif
