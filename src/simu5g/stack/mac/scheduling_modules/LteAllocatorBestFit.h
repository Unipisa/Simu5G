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

#ifndef _LTE_LTEALLOCATORBESTFIT_H_
#define _LTE_LTEALLOCATORBESTFIT_H_

#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/allocator/LteAllocatorUtils.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/conflict_graph/ConflictGraph.h"

namespace simu5g {

struct Candidate {
    Band index;
    unsigned int len;
    bool greater;
};

class LteAllocatorBestFit : public virtual LteScheduler
{
  protected:

    typedef SortedDesc<MacCid, unsigned int> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

    ConflictGraph *conflictGraph_ = nullptr;

    /**
     * e.g. allocatedRbsBand_ [ <plane> ] [ <antenna> ] [ <band> ] gives the amount of blocks allocated for each UE
     */
    std::vector<std::vector<AllocatedRbsPerBandMapA>> allocatedRbsPerBand_;

    // For each UE, stores the amount of blocks allocated for each band
    AllocatedRbsPerUeMapA allocatedRbsUe_;

    /**
     * Amount of Blocks allocated during this TTI, for each plane and for each antenna
     *
     * e.g. allocatedRbsMatrix_[ <plane> ] [ <antenna> ]
     */
    std::vector<std::vector<unsigned int>> allocatedRbsMatrix_;

    // Map that specifies, for each NodeId, which bands are free and which are not
    std::map<MacNodeId, std::map<Band, bool>> perUEbandStatusMap_;

    typedef std::pair<AllocationUeType, std::set<MacNodeId>> AllocationType_Set;
    // Map that specifies which bands can (non-exclusive bands-D2D) or cannot (exclusive bands-CELL) be shared
    std::map<Band, AllocationType_Set> bandStatusMap_;

    /**
     * Enumerator specified for the return of mutualExclusiveAllocation() function.
     * @see mutualExclusiveAllocation()
     */

    // returns the next "hole" in the subframe where the UEs can be eventually allocated
    void checkHole(Candidate& candidate, Band holeIndex, unsigned int holeLen, unsigned int req);

    // returns true if the two nodes cannot transmit on the same block
    bool checkConflict(const CGMatrix *cgMatrix, MacNodeId nodeIdA, MacNodeId nodeIdB);

  public:

    LteAllocatorBestFit(Binder *binder);

    void prepareSchedule() override;

    void commitSchedule() override;

    // *****************************************************************************************

    // Initialize the allocation structures of the Scheduler
    void initAndReset();

    // Set the specified bands to exclusive
    void setAllocationType(std::vector<Band> bandVect, AllocationUeType type, MacNodeId nodeId);
};

} //namespace

#endif // _LTE_LTEALLOCATORBESTFIT_H_

