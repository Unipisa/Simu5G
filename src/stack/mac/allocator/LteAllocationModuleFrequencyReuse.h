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

#ifndef _LTE_LTEALLOCATIONMODULEFREQUENCYREUSE_H_
#define _LTE_LTEALLOCATIONMODULEFREQUENCYREUSE_H_

#include "common/LteCommon.h"
#include "stack/mac/allocator/LteAllocationModule.h"

class LteAllocationModuleFrequencyReuse : public LteAllocationModule
{
    public:
    /// Default constructor.
    LteAllocationModuleFrequencyReuse(LteMacEnb *mac, const Direction direction);
    // Store the Allocation based on passed paremeter
    virtual void storeAllocation( std::vector<std::vector<AllocatedRbsPerBandMapA> > allocatedRbsPerBand,std::set<Band>* untouchableBands = nullptr);
    // Get the bands already allocated by RAC and RTX ( Debug purpose)
    virtual std::set<Band> getAllocatorOccupiedBands();
};

#endif
