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

#include "simu5g/stack/mac/allocator/LteAllocationModule.h"
#include "simu5g/stack/mac/LteMacEnb.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

LteAllocationModule::LteAllocationModule(LteMacEnb *mac, Direction direction) : mac_(mac), dir_(direction)
{
}

void LteAllocationModule::init(const unsigned int resourceBlocks, const unsigned int bands)
{
    // clear the OFDMA available blocks and set available planes to 1 (just the main OFDMA space)
    totalRbsMatrix_.clear();
    totalRbsMatrix_.resize(MAIN_PLANE + 1);
    // set the available antennas of MAIN plane to 1 (just MACRO antenna)
    totalRbsMatrix_.at(MAIN_PLANE).resize(MACRO + 1);
    // initialize main OFDMA space
    totalRbsMatrix_[MAIN_PLANE][MACRO] = resourceBlocks;

    // initialize number of bands
    bands_ = bands;

    // clear the OFDMA allocated blocks and set available planes to 1 (just the main OFDMA space)
    allocatedRbsMatrix_.clear();
    allocatedRbsMatrix_.resize(MAIN_PLANE + 1);
    // set the available antennas of MAIN plane to 1 (just MACRO antenna)
    allocatedRbsMatrix_.at(MAIN_PLANE).resize(MACRO + 1, 0);

    // clean and copy stored block-allocation info
    prevAllocatedRbsPerBand_.clear();
    prevAllocatedRbsPerBand_ = allocatedRbsPerBand_;

    // clear and reinitialize the allocatedBlocks structures and set available planes to 1 (just the main OFDMA space)
    allocatedRbsPerBand_.clear();
    allocatedRbsPerBand_.resize(MAIN_PLANE + 1);
    // set the available antennas of MAIN plane to 1 (just MACRO antenna)
    allocatedRbsPerBand_.at(MAIN_PLANE).resize(MACRO + 1);

    // clear and reinitialize the freeResourceBlocks vector and set available planes to 1 (just the main OFDMA space)
    freeRbsMatrix_.clear();
    freeRbsMatrix_.resize(MAIN_PLANE + 1);
    // set the available antennas of MAIN plane to 1 (just MACRO antenna)
    freeRbsMatrix_.at(MAIN_PLANE).resize(MACRO + 1);
    // initializes all free blocks for each band in the main plane, MACRO antenna.
    freeRbsMatrix_.at(MAIN_PLANE).at(MACRO).resize(bands_, 0);

    // clear UE,LB Map
    allocatedRbsUe_.clear();
}

void LteAllocationModule::reset(const unsigned int resourceBlocks, const unsigned int bands)
{
    // clean and copy stored block-allocation info
    prevAllocatedRbsPerBand_ = allocatedRbsPerBand_;

    // reset structures only if they were used in the previous time slot
    if (usedInLastSlot_) {
        // initialize main OFDMA space
        for (auto& plane : totalRbsMatrix_) {
            for (auto& antenna : plane) {
                antenna = resourceBlocks;
            }
        }

        // set the available antennas of MAIN plane to 1 (just MACRO antenna)
        for (auto& plane : allocatedRbsMatrix_) {
            for (auto& antenna : plane) {
                antenna = 0;
            }
        }

        // clear and reinitialize the allocatedBlocks structures and set available planes to 1 (just the main OFDMA space)
        for (auto& plane : allocatedRbsPerBand_) {
            for (auto& antenna : plane) {
                antenna.clear();
            }
        }

        // clear and reinitialize the freeResourceBlocks vector and set available planes to 1 (just the main OFDMA space)
        for (auto& plane : freeRbsMatrix_) {
            for (auto& antenna : plane) {
                for (auto& band : antenna) {
                    band = 0;
                }
            }
        }

        // clear UE,LB Map
        allocatedRbsUe_.clear();
    }

    usedInLastSlot_ = false;
}


void LteAllocationModule::ensureNodeInitialized(const MacNodeId nodeId)
{
    // Ensure the allocatedRbsUe_ map has an entry for this node
    // Using operator[] creates a default-initialized entry if it doesn't exist
    allocatedRbsUe_[nodeId];
}

unsigned int LteAllocationModule::computeTotalRbs()
{
    // The amount of available blocks in all the OFDM spaces is obtained from the overall
    // amount of RBs in every plane of every antenna, decreased by the number of allocated blocks

    int resourceBlocks = 0;
    int allocatedBlocks = 0;

    // for each plane
    for (unsigned int i = 0; i < totalRbsMatrix_.size(); ++i) {
        // for each antenna
        for (unsigned int j = 0; j < totalRbsMatrix_.at(i).size(); ++j) {
            resourceBlocks += totalRbsMatrix_.at(i).at(j);
            allocatedBlocks += allocatedRbsMatrix_.at(i).at(j);
        }
    }

    int availableBlocks = resourceBlocks - allocatedBlocks;

    // available blocks MUST be a non-negative amount
    if (availableBlocks < 0)
        throw cRuntimeError("LteAllocator::checkOFDMspace(): Negative OFDM space");

    // DEBUG
    EV << NOW << " LteAllocationModule::computeTotalRbs " << dirToA(dir_) << " - total " << resourceBlocks <<
        ", allocated " << allocatedBlocks << ", " << availableBlocks << " blocks available" << endl;

    return (unsigned int)availableBlocks;
}

unsigned int LteAllocationModule::availableBlocks(const MacNodeId nodeId, const Remote antenna, const Band band)
{
    Plane plane = MAIN_PLANE;

    // blocks allocated in the current band
    unsigned int allocatedBlocks = allocatedRbsPerBand_[plane][antenna][band].allocated_;

    if (allocatedBlocks == 0) {
        EV << NOW << " LteAllocator::availableBlocks " << dirToA(dir_) << " - Band " << band << " has 1 block available" << endl;
        return 1;
    }

    // no space available on current antenna.
    return 0;
}

unsigned int LteAllocationModule::getAllocatedBlocks(Plane plane, const Remote antenna, const Band band)
{
    return allocatedRbsPerBand_[plane][antenna][band].allocated_;
}

unsigned int LteAllocationModule::getInterferingBlocks(Plane plane, const Remote antenna, const Band band)
{
    if (!prevAllocatedRbsPerBand_.empty())
        return prevAllocatedRbsPerBand_[plane][antenna][band].allocated_;
    else
        return 1000;
}

unsigned int LteAllocationModule::availableBlocks(const MacNodeId nodeId, const Plane plane, const Band band)
{
    ensureNodeInitialized(nodeId);

    // compute available blocks on all antennas for the given user and plane.
    RemoteSet antennas = allocatedRbsUe_.at(nodeId).availableAntennaSet_;
    unsigned int available = 0;

    for (const auto& antenna : antennas) {
        available += availableBlocks(nodeId, antenna, band);
    }
    // returning available blocks
    return available;
}

bool LteAllocationModule::addBlocks(const Band band, const MacNodeId nodeId, const unsigned int blocks,
        const unsigned int bytes)
{
    ensureNodeInitialized(nodeId);

    // all antennas for the given user and plane.
    RemoteSet antennas = allocatedRbsUe_.at(nodeId).availableAntennaSet_;
    bool ret = false;
    for (const auto& antenna : antennas) {
        ret = addBlocks(antenna, band, nodeId, blocks, bytes);
        if (ret)
            break;
    }
    // returning allocation result
    return ret;
}

bool LteAllocationModule::addBlocks(const Remote antenna, const Band band, const MacNodeId nodeId,
        const unsigned int blocks, const unsigned int bytes)
{
    // Check if the band exists
    if (band >= bands_)
        throw cRuntimeError("LteAllocator::addBlocks(): Invalid band %d", (int)band);

    // Check if there's enough OFDM space
    // retrieving user's plane
    Plane plane = MAIN_PLANE;

    // Obtain the available blocks on the given band
    int availableBlocksOnBand = availableBlocks(nodeId, antenna, band);

    // Check if the band can satisfy the request
    if (availableBlocksOnBand == 0) {
        EV << NOW << " LteAllocator::addBlocks " << dirToA(dir_) << " - Node " << nodeId <<
            ", not enough space on band " << band << ": requested " << blocks <<
            " available " << availableBlocksOnBand << " " << endl;
        return false;
    }
    // check if UE is out of range. (CQI=0 => bytes=0)
    if (bytes == 0) {
        EV << NOW << " LteAllocator::addBlocks " << dirToA(dir_) << " - Node " << nodeId << " - 0 bytes available with " << blocks << " blocks" << endl;
        return false;
    }

    // Note the request on the allocator structures
    allocatedRbsPerBand_[plane][antenna][band].ueAllocatedRbsMap_[nodeId] += blocks;
    allocatedRbsPerBand_[plane][antenna][band].ueAllocatedBytesMap_[nodeId] += bytes;
    allocatedRbsPerBand_[plane][antenna][band].allocated_ += blocks;

    allocatedRbsUe_[nodeId].ueAllocatedRbsMap_[antenna][band] += blocks;
    allocatedRbsUe_[nodeId].allocatedBlocks_ += blocks;
    allocatedRbsUe_[nodeId].allocatedBytes_ += bytes;
    allocatedRbsUe_[nodeId].antennaAllocatedRbs_[antenna] += blocks;

    // Store the request in the allocationList
    AllocationElem elem;
    elem.resourceBlocks_ = blocks;
    elem.bytes_ = bytes;
    allocatedRbsUe_[nodeId].allocationMap_[antenna][band].push_back(elem);

    // update the allocatedBlocks counter
    allocatedRbsMatrix_[plane][antenna] += blocks;

    usedInLastSlot_ = true;

    EV << NOW << " LteAllocator::addBlocks " << dirToA(dir_) << " - Node " << nodeId << ", the request of " << blocks << " blocks on band " << band << " satisfied" << endl;

    return true;
}

unsigned int LteAllocationModule::removeBlocks(const Remote antenna, const Band band, const MacNodeId nodeId)
{
    // Check if the band exists
    if (band >= bands_) {
        EV << NOW << " LteAllocator::removeBlocks " << dirToA(dir_) << " - Node " << nodeId << ", invalid band " << band << endl;
        return 0;
    }

    ensureNodeInitialized(nodeId);

    // Retrieving user's plane
    Plane plane = MAIN_PLANE;

    unsigned int toDrain = allocatedRbsPerBand_[plane][antenna][band].ueAllocatedRbsMap_[nodeId];

    // If the number of blocks allocated by the nodeId in the band is zero, do nothing!
    if (toDrain == 0)
        return toDrain;

    // Note the removal in the allocator structures
    allocatedRbsPerBand_[plane][antenna][band].allocated_ -= toDrain;
    allocatedRbsUe_[nodeId].allocatedBlocks_ -= toDrain;

    allocatedRbsUe_[nodeId].ueAllocatedRbsMap_[antenna][band] = 0;
    allocatedRbsUe_[nodeId].allocatedBytes_ = 0;
    allocatedRbsPerBand_[plane][antenna][band].ueAllocatedRbsMap_[nodeId] = 0;

    // Drop the allocation list
    allocatedRbsUe_[nodeId].allocationMap_[antenna][band].clear();

    // Update the allocatedBlocks counter
    allocatedRbsMatrix_[plane][antenna] -= toDrain;

    usedInLastSlot_ = true;

    // DEBUG
    EV << NOW << " LteAllocator::removeBlocks " << dirToA(dir_) << " - Node " << nodeId << ", " << toDrain << " blocks drained from band " << band << endl;

    return toDrain;
}

unsigned int LteAllocationModule::rbOccupation(const MacNodeId nodeId, RbMap& rbMap)
{
    ensureNodeInitialized(nodeId);
    // Compute allocated blocks on all antennas for the given user and logical band.
    RemoteSet antennas = allocatedRbsUe_.at(nodeId).availableAntennaSet_;

    unsigned int blocks = 0;

    for (const auto& antenna : antennas) {
        for (Band b = 0; b < bands_; ++b) {
            blocks += (rbMap[antenna][b] = getBlocks(antenna, b, nodeId));
        }
    }
    return blocks;
}

} //namespace

