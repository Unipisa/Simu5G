//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
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

    // reference to background base station
    BackgroundBaseStation* bgBaseStation_;

    // reference to class AMC for this cell
    BackgroundCellAmc* bgAmc_;

  public:
    BackgroundCellTrafficManager();
    virtual ~BackgroundCellTrafficManager();
    virtual void initialize(int stage);

    // returns the CQI based on the given position and power
    virtual Cqi computeCqi(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower = 0.0);

    // returns the bytes per block of the given UE for in the given direction
    virtual  unsigned int getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir);

    // signal that the RAC for the given UE has been handled
    virtual void racHandled(MacNodeId bgUeId);
};

#endif
