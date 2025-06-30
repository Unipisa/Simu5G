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

#ifndef __EXTCELL_H_
#define __EXTCELL_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>

#include "common/LteCommon.h"
#include "common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

typedef std::vector<int> BandStatus;

/* The allocation type defines the interference produced by the external cell
 * - FULL_ALLOC: the cell allocates all RBs in the frame
 * - RANDOM_ALLOC: the cell allocates X RBs, which are chosen randomly
 * - CONTIGUOUS_ALLOC: the cell allocates X contiguous RBs, starting from a given RB
 */
enum BandAllocationType {
    FULL_ALLOC, RANDOM_ALLOC, CONTIGUOUS_ALLOC
};

class ExtCell : public cSimpleModule
{
    // Playground coordinates
    inet::Coord position_;

    // ID among all the external cells
    int id_;

    // TX power
    double txPower_;

    // TX direction
    TxDirectionType txDirection_;

    // TX angle
    double txAngle_;

    // Carrier frequency
    double carrierFrequency_;

    // Number of logical bands
    unsigned int numBands_;

    // Reference to the binder
    inet::ModuleRefByPar<Binder> binder_;

    // Current and previous band occupation status. Used for interference computation
    BandStatus bandStatus_;
    BandStatus prevBandStatus_;

    // TTI self message
    cMessage *ttiTick_ = nullptr;

    /*** ALLOCATION MANAGEMENT ***/

    // Allocation Type for this cell
    BandAllocationType allocationType_;

    // Percentage of RBs allocated by the external cell
    // Used for RANDOM_ALLOC and CONTIGUOUS_ALLOC allocation types
    double bandUtilization_;

    // Index of the first allocated RB for CONTIGUOUS_ALLOC allocation type
    int startingOffset_;

    // Update the band status. Called at each TTI (not used for FULL_ALLOC)
    void updateBandStatus();

    // Move the current status in the prevBandStatus structure and reset the former
    void resetBandStatus();
    /*****************************/

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::INITSTAGE_LOCAL + 2; }
    void handleMessage(cMessage *msg) override;

  public:

    const inet::Coord getPosition() { return position_; }

    int getId() { return id_; }

    double getTxPower() { return txPower_; }

    TxDirectionType getTxDirection() { return txDirection_; }

    double getTxAngle() { return txAngle_; }

    unsigned int getNumBands() { return numBands_; }

    void setBlock(int band) { bandStatus_.at(band) = 1; }

    void unsetBlock(int band) { bandStatus_.at(band) = 0; }

    int getBandStatus(int band) { return bandStatus_.at(band); }

    int getPrevBandStatus(int band) { return prevBandStatus_.at(band); }

    // Set the band utilization percentage
    void setBandUtilization(double bandUtilization);
};

} //namespace

#endif

