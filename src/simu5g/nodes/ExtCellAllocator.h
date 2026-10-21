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

#ifndef __EXTCELLALLOCATOR_H_
#define __EXTCELLALLOCATOR_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/stack/phy/radio/CellularPhantomRadio.h"

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

/**
 * See the NED documentation of ExtCellAllocator.
 */
class ExtCellAllocator : public cSimpleModule
{
    MacNodeId nodeId_ = NODEID_NONE; // the node id its transmissions on the radio medium name it by

    // the cell's phantom radio
    inet::ModuleRefByPar<CellularPhantomRadio> radio_;

    // Carrier frequency
    GHz carrierFrequency_;

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

    // registers with the radio medium the transmission the current band status stands for, lasting the given time
    void registerTransmission(simtime_t duration);
    /*****************************/

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

  public:

    MacNodeId getNodeId() const { return nodeId_; }

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
