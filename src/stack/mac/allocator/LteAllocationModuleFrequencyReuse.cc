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

#include "stack/mac/allocator/LteAllocationModuleFrequencyReuse.h"
#include "stack/mac/layer/LteMacEnb.h"

namespace simu5g {

using namespace inet;

LteAllocationModuleFrequencyReuse::LteAllocationModuleFrequencyReuse(LteMacEnb *mac, Direction direction)
    : LteAllocationModule(mac, direction)
{
}

void LteAllocationModuleFrequencyReuse::storeAllocation(std::vector<std::vector<AllocatedRbsPerBandMapA>> allocatedRbsPerBand, std::set<Band> *untouchableBands)
{
    Plane plane = MAIN_PLANE;
    const Remote antenna = MACRO;
    std::map<std::pair<MacNodeId, Band>, std::pair<unsigned int, unsigned int>> NodeIdRbsBytesMap;
    NodeIdRbsBytesMap.clear();
    // Create an empty vector
    if (untouchableBands == nullptr) {
        std::set<Band> tempBand;
        untouchableBands = &tempBand;
        untouchableBands->clear();
    }

    for (unsigned int band = 0; band < bands_; band++) {
        // Skip allocation if the band is untouchable (this means that the information is already allocated)
        if (untouchableBands->find(band) == untouchableBands->end()) {
            // Copy the ueAllocatedRbsMap
            auto it_ext = allocatedRbsPerBand[plane][antenna][band].ueAllocatedRbsMap_.begin();
            auto et_ext = allocatedRbsPerBand[plane][antenna][band].ueAllocatedRbsMap_.end();
            auto it2_ext = allocatedRbsPerBand[plane][antenna][band].ueAllocatedBytesMap_.begin();
            auto et2_ext = allocatedRbsPerBand[plane][antenna][band].ueAllocatedBytesMap_.end();

            while (it_ext != et_ext && it2_ext != et2_ext) {
                allocatedRbsPerBand_[plane][antenna][band].ueAllocatedRbsMap_[it_ext->first] = it_ext->second;    // Blocks
                allocatedRbsPerBand_[plane][antenna][band].ueAllocatedBytesMap_[it_ext->first] = it2_ext->second; // Bytes

                // Creates a pair (a key) for the Map
                std::pair<MacNodeId, Band> Key_pair(it_ext->first, band);
                // Creates a pair for the blocks and bytes values
                std::pair<unsigned int, unsigned int> Value_pair(it_ext->second, it2_ext->second);
                //Store the nodeId RBs
                NodeIdRbsBytesMap[Key_pair] = Value_pair;
                it_ext++;
                it2_ext++;
            }
            // Copy the allocatedRbsPerBand
            allocatedRbsPerBand_[plane][antenna][band].allocated_ = allocatedRbsPerBand[plane][antenna][band].allocated_;

            if (allocatedRbsPerBand[plane][antenna][band].allocated_ > 0)
                allocatedRbsMatrix_[MAIN_PLANE][MACRO]++;
        }
    }

    for (const auto &[key, value] : NodeIdRbsBytesMap) {
        // Skip allocation if the band is untouchable (this means that the information is already allocated)
        if (untouchableBands->find(key.second) == untouchableBands->end()) {
            allocatedRbsUe_[key.first].ueAllocatedRbsMap_[antenna][key.second] = value.first; //Blocks
            allocatedRbsUe_[key.first].allocatedBlocks_ += value.first; //Blocks
            allocatedRbsUe_[key.first].allocatedBytes_ += value.second; //Bytes

            // Creates and store the allocation Elem
            AllocationElem elem;
            elem.resourceBlocks_ = value.first;
            elem.bytes_ = value.second;

            allocatedRbsUe_[key.first].allocationMap_[antenna][key.second].push_back(elem);
        }
    }

    usedInLastSlot_ = true;
}

std::set<Band> LteAllocationModuleFrequencyReuse::getAllocatorOccupiedBands()
{
    // TODO add support for logical band different from the number of real bands
    std::set<Band> vectorBand;
    vectorBand.clear();
    for (unsigned int i = 0; i < bands_; i++) {
        if (allocatedRbsPerBand_[MAIN_PLANE][MACRO][i].allocated_ > 0) vectorBand.insert(i);
    }
    return vectorBand;
}

} //namespace

