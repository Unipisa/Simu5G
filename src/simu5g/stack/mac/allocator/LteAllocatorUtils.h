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

#ifndef LTEALLOCATORUTILS_H_
#define LTEALLOCATORUTILS_H_

namespace simu5g {

//----------------------------------------------------------------------
//             STRUCT AND TYPE_DEF FOR allocatedRbsPerBand_
//----------------------------------------------------------------------
typedef std::map<MacNodeId, unsigned int> UeAllocatedBlocksMapA;
typedef std::map<MacNodeId, unsigned int> UeAllocatedBytesMapA;
/// This structure contains information for a single band
struct AllocatedRbsPerBandInfo
{
    /// Stores for each UE the amount of blocks allocated in the structure band
    UeAllocatedBlocksMapA ueAllocatedRbsMap_;
    /// Stores for each UE the amount of bytes allocated in the structure band
    UeAllocatedBytesMapA ueAllocatedBytesMap_;

    /// Stores the amount of blocks allocated to every UE in the structure band
    unsigned int allocated_ = 0;

    AllocatedRbsPerBandInfo()
    {
    }

};

typedef std::map<Band, AllocatedRbsPerBandInfo> AllocatedRbsPerBandMapA;

//----------------------------------------------------------------------
//             STRUCT AND TYPE_DEF FOR allocatedRbsUe_
//----------------------------------------------------------------------

// Amount of bytes served by a single set of allocated blocks
struct AllocationElem
{
    /// Blocks of the request
    unsigned int resourceBlocks_;
    /// Bytes served
    unsigned int bytes_;
};

typedef std::list<AllocationElem> AllocationListA;
typedef std::map<Band, AllocationListA> PerBandAllocationMapA;
typedef std::map<Band, unsigned int> PerBandAllocatedRbsMapA;

/// This structure contains information for a single UE
struct AllocatedRbsPerUeInfo
{
    /// Stores the amount of blocks allocated in every band by the structure UE
    unsigned int allocatedBlocks_ = 0;
    /// Stores the amount of bytes allocated to every UE in the structure band
    unsigned int allocatedBytes_ = 0;

    // if false this user is not using MU-MIMO
    bool muMimoEnabled_ = false;
    // if false this user transmits on MAIN_PLANE, otherwise it is considered as secondary
    bool secondaryUser_ = false;
    MacNodeId peerId_ = NODEID_NONE;

    // amount of blocks allocated for this UE for each remote and for each band
    std::map<Remote, PerBandAllocatedRbsMapA> ueAllocatedRbsMap_;
    /// When an allocation is performed, the amount of blocks requested and the amount of bytes is registered into this list
    std::map<Remote, PerBandAllocationMapA> allocationMap_;
    // amount of blocks allocated for this UE for each remote
    std::map<Remote, unsigned int> antennaAllocatedRbs_;

    // antennas available for this user
    RemoteSet availableAntennaSet_;

    // first available antenna
    Remote currentAntenna_ = MACRO;

  public:

    AllocatedRbsPerUeInfo()
    {
        availableAntennaSet_.insert(MACRO);
    }

};

typedef std::map<MacNodeId, AllocatedRbsPerUeInfo> AllocatedRbsPerUeMapA;

/**
 *  Type used to discriminate the UEs in the allocation phase.
 */
enum AllocationUeType
{
    EXCLUSIVE,
    SHARED,
    UNUSED,
};

} // namespace simu5g

#endif /* LTEALLOCATORUTILS_H_ */

