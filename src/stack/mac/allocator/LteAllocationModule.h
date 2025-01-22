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

#ifndef _LTE_LTEALLOCATIONMODULE_H_
#define _LTE_LTEALLOCATIONMODULE_H_

#include "common/LteCommon.h"
#include "stack/mac/allocator/LteAllocatorUtils.h"

#include "stack/mac/LteMacEnb.h"

namespace simu5g {

using namespace omnetpp;

class LteMacEnb;

class LteAllocationModule
{
    // Members *********************************************************************************

    // --- General Parameters ------------------------------------------------------------------

  protected:

    /// Owner MAC module
    opp_component_ptr<LteMacEnb> mac_;

    /// Number of bands
    unsigned int bands_ = 0;

    /// Operational Direction. Set via initialize().
    Direction dir_;

    /// Flag that indicates when the data structures need to be reset in the next slot
    bool usedInLastSlot_ = false;

    /*
     * We will consider the following two planes
     *
     * plane
     *  [0]      represents the main OFDMA space
     *  [1]      the space obtained by summing all the Mu-Mimo spaces
     */

    // *************************** RESOURCE BLOCKS MATRIX ***************************
    /**
     * Total amount of resource blocks, for each plane and for each antenna
     *
     * e.g. totalRbsMatrix_[ <plane> ] [ <antenna> ]
     */
    std::vector<std::vector<unsigned int>> totalRbsMatrix_;

    /**
     * Amount of blocks allocated during this TTI, for each plane and for each antenna
     *
     * e.g. allocatedRbsMatrix_[ <plane> ] [ <antenna> ]
     */
    std::vector<std::vector<unsigned int>> allocatedRbsMatrix_;

    /**
     * Amount of available resource blocks, for each plane, for each antenna and for each band
     *
     * e.g. availableRbsMatrix_[ <plane> ] [ <antenna> ] [ <band> ]
     */
    std::vector<std::vector<std::vector<unsigned int>>> freeRbsMatrix_;

    /************************************************************
    *    From UE to Logical Band
    ************************************************************/

  public:

    // Amount of bytes served by a single set of allocated blocks
    struct AllocationElem
    {
        /// Blocks of the request
        unsigned int resourceBlocks_;
        /// Bytes served
        unsigned int bytes_;
    };

    typedef std::list<AllocationElem> AllocationList;

    typedef std::map<Band, AllocationList> PerBandAllocationMap;

    typedef std::map<Band, unsigned int> PerBandAllocatedRbsMap;

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
        std::map<Remote, PerBandAllocatedRbsMap> ueAllocatedRbsMap_;
        /// When an allocation is performed, the amount of blocks requested and the amount of bytes is registered into this list
        std::map<Remote, PerBandAllocationMap> allocationMap_;
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

    typedef std::map<MacNodeId, AllocatedRbsPerUeInfo> AllocatedRbsPerUeMap;

    // b) Variables

  protected:

    /// For each UE, stores the amount of blocks allocated for each band
    AllocatedRbsPerUeMap allocatedRbsUe_;

    /************************************************************
    *   From Logical Bands to UE
    ************************************************************/

  public:

    typedef std::map<MacNodeId, unsigned int> UeAllocatedBlocksMap;
    typedef std::map<MacNodeId, unsigned int> UeAllocatedBytesMap;

    /// This structure contains information for a single band
    struct AllocatedRbsPerBandInfo
    {
        /// Stores for each UE the amount of blocks allocated in the structure band
        UeAllocatedBlocksMap ueAllocatedRbsMap_;
        /// Stores for each UE the amount of bytes allocated in the structure band
        UeAllocatedBytesMap ueAllocatedBytesMap_;

        /// Stores the amount of blocks allocated to every UE in the structure band
        unsigned int allocated_ = 0;

        AllocatedRbsPerBandInfo()
        {
        }
    };

    typedef std::map<Band, AllocatedRbsPerBandInfo> AllocatedRbsPerBandMap;

  protected:

    /**
     * e.g. allocatedRbsBand_ [ <plane> ] [ <antenna> ] [ <band> ] give the amount of blocks allocated for each UE
     */
    std::vector<std::vector<AllocatedRbsPerBandMap>> allocatedRbsPerBand_;

    /*
     * Stores the block-allocation info of the previous TTI in order to use it for interference computation
     */
    std::vector<std::vector<AllocatedRbsPerBandMap>> prevAllocatedRbsPerBand_;

  public:

    /// Default constructor.
    LteAllocationModule(LteMacEnb *mac, const Direction direction);

    /// Destructor.
    virtual ~LteAllocationModule() {}

    // init Allocation Module structure
    void init(const unsigned int resourceBlocks, const unsigned int bands);

    // reset Allocation Module structure
    void reset(const unsigned int resourceBlocks, const unsigned int bands);

    // ********* MU-MIMO Support *********
    // Configure MuMimo between "nodeId" and "peer"
    bool configureMuMimoPeering(const MacNodeId nodeId, const MacNodeId peer);

    // MU-MIMO configuration functions
    void configureOFDMplane(const Plane plane);
    void setRemoteAntenna(const Plane plane, const Remote antenna);
    Plane getOFDMPlane(const MacNodeId nodeId);

    // returns the Mu-MIMO peer id if it exists, own id otherwise
    MacNodeId getMuMimoPeer(const MacNodeId nodeId) const;
    // **********************************

    // ************** Resource Blocks Allocation Status **************
    // Returns the amount of available blocks in the whole system
    unsigned int computeTotalRbs();

    // returns the amount of free blocks for the given band in the given plane
    unsigned int availableBlocks(const MacNodeId nodeId, const Plane plane, const Band band);

    // returns the amount of free blocks for the given band and for the given antenna
    unsigned int availableBlocks(const MacNodeId nodeId, const Remote antenna, const Band band);
    // ***************************************************************

    // ************** Resource Blocks Allocation Methods **************
    // tries to satisfy the resource block request in the given band and for the given antenna
    bool addBlocks(const Remote antenna, const Band band, const MacNodeId nodeId, const unsigned int blocks,
            const unsigned int bytes);

    // tries to satisfy the resource block request in the first available antenna
    bool addBlocks(const Band band, const MacNodeId nodeId, const unsigned int blocks, const unsigned int bytes);

    // remove resource Blocks previously allocated in a band by a UE
    unsigned int removeBlocks(const Remote antenna, const Band band, const MacNodeId nodeId);
    // ****************************************************************

    // --- Get (Parameters) --------------------------------------------------------------------

    // Simple block-amount returning methods

    /**
     * Returns the amount of blocks allocated by a UE in a Band
     * @param antenna The MACRO or DAS antenna.
     * @param band the logical band
     * @param nodeId the node id of the user
     * @return amount of blocks allocated
     */
    unsigned int getBlocks(const Remote antenna, const Band band, const MacNodeId nodeId)
    {
        Plane plane = allocatedRbsUe_[nodeId].secondaryUser_ ? MU_MIMO_PLANE : MAIN_PLANE;
        return allocatedRbsPerBand_[plane][antenna][band].ueAllocatedRbsMap_[nodeId];
    }

    /*
     * Returns the amount of blocks allocated in a Band
     */
    unsigned int getAllocatedBlocks(Plane plane, const Remote antenna, const Band band);
    unsigned int getInterferingBlocks(Plane plane, const Remote antenna, const Band band);

    unsigned int getBytes(const Remote antenna, const Band band, const MacNodeId nodeId)
    {
        Plane plane = allocatedRbsUe_[nodeId].secondaryUser_ ? MU_MIMO_PLANE : MAIN_PLANE;
        return allocatedRbsPerBand_[plane][antenna][band].ueAllocatedBytesMap_[nodeId];
    }

    // computes the amount of blocks allocated by the given UE
    unsigned int getBlocks(const MacNodeId nodeId)
    {
        return allocatedRbsUe_[nodeId].allocatedBlocks_;
    }

    // computes the amount of blocks allocated for the given plane and the given antenna
    unsigned int getBlocks(const Plane plane, const Remote antenna)
    {
        return allocatedRbsMatrix_[plane][antenna];
    }

    unsigned int rbOccupation(const MacNodeId nodeId, RbMap& rbMap);

    // --------- Map Iteration Methods --------->
    AllocatedRbsPerUeMap::const_iterator getAllocatedBlocksUeBegin()
    {
        return allocatedRbsUe_.begin();
    }

    AllocatedRbsPerUeMap::const_iterator getAllocatedBlocksUeEnd()
    {
        return allocatedRbsUe_.end();
    }

    std::vector<std::vector<unsigned int>>::const_iterator getAllocatedBlocksBegin()
    {
        return allocatedRbsMatrix_.begin();
    }

    std::vector<std::vector<unsigned int>>::const_iterator getAllocatedBlocksEnd()
    {
        return allocatedRbsMatrix_.end();
    }

    //  Band Allocation Map
    AllocationList::const_iterator getAllocatedBlocksUeAllocationListBegin(const Remote antenna, const Band b,
            const MacNodeId nodeId)
    {
        return allocatedRbsUe_[nodeId].allocationMap_[antenna][b].begin();
    }

    AllocationList::const_iterator getAllocatedBlocksUeAllocationListEnd(const Remote antenna, const Band b,
            const MacNodeId nodeId)
    {
        return allocatedRbsUe_[nodeId].allocationMap_[antenna][b].end();
    }

    /*
     *  Support for allocation with frequency reuse
     */

    // Store the Allocation based on passed parameters
    virtual void storeAllocation(std::vector<std::vector<AllocatedRbsPerBandMapA>> allocatedRbsPerBand, std::set<Band> *untouchableBands = nullptr)
    {
        return;
    }

    // Get the bands already allocated
    virtual std::set<Band> getAllocatorOccupiedBands()
    {
        std::set<Band> bandVector;
        return bandVector;
    }

    // returns the number of logical bands
    unsigned int getNumBands()
    {
        return bands_;
    }

};

} //namespace

#endif

