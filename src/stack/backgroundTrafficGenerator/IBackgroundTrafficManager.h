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

#ifndef IBACKGROUNDTRAFFICMANAGER_H_
#define IBACKGROUNDTRAFFICMANAGER_H_

#include <list>
#include <vector>

#include <inet/common/geometry/common/Coord.h>

#include "stack/backgroundTrafficGenerator/generators/TrafficGeneratorBase.h"
#include "common/LteCommon.h"
#include "common/LteCommonEnum_m.h"

namespace simu5g {

//
// IBackgroundTrafficManager
//
class IBackgroundTrafficManager
{
  public:

    // Set carrier frequency
    virtual void setCarrierFrequency(double carrierFrequency) = 0;

    // Get the number of RBs
    virtual unsigned int getNumBands() = 0;

    // Get the tx power of the BS
    virtual double getBsTxPower() = 0;

    // Get the position of the BS
    virtual inet::Coord getBsCoord() = 0;

    // Invoked by the UE's traffic generator when new data is backlogged
    virtual void notifyBacklog(int index, Direction dir, bool rtx = false) = 0;

    // Returns the CQI based on the given position and power
    virtual Cqi computeCqi(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower = 0.0) = 0;

    // Returns the CQI based on the given SINR
    virtual Cqi computeCqiFromSinr(double sinr) = 0;

    // Returns the pointer to the traffic generator of the given background UE
    virtual TrafficGeneratorBase *getTrafficGenerator(MacNodeId bgUeId) = 0;

    // Returns the begin (end) iterator of the vector of UEs
    virtual std::vector<TrafficGeneratorBase *>::const_iterator getBgUesBegin() = 0;
    virtual std::vector<TrafficGeneratorBase *>::const_iterator getBgUesEnd() = 0;

    // Returns the begin (end) iterator of the vector of backlogged UEs
    virtual std::list<int>::const_iterator getBackloggedUesBegin(Direction dir, bool rtx = false) = 0;
    virtual std::list<int>::const_iterator getBackloggedUesEnd(Direction dir, bool rtx = false) = 0;

    // Returns the begin (end) iterator of the vector of backlogged UEs that are waiting for RAC handshake to finish
    virtual std::list<int>::const_iterator getWaitingForRacUesBegin() = 0;
    virtual std::list<int>::const_iterator getWaitingForRacUesEnd() = 0;

    // Returns the buffer of the given UE in the given direction
    virtual unsigned int getBackloggedUeBuffer(MacNodeId bgUeId, Direction dir, bool rtx = false) = 0;

    // Returns the bytes per block of the given UE in the given direction
    virtual unsigned int getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir) = 0;

    // Signal that the RAC for the given UE has been handled
    virtual void racHandled(MacNodeId bgUeId) = 0;

    // Update background UE's backlog and return true if the buffer is empty
    virtual unsigned int consumeBackloggedUeBytes(MacNodeId bgUeId, unsigned int bytes, Direction dir, bool rtx = false) = 0;

    // Compute received power for a background UE according to path loss
    virtual double getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus) = 0;
};

} //namespace

#endif

