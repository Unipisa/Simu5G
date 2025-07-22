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

#include "simu5g/stack/compManager/compManagerProportional/X2CompProportionalReplyIE_m.h"

namespace simu5g {

void X2CompProportionalReplyIE::setAllowedBlocksMap(std::vector<CompRbStatus>& map)
{
    size_t oldSize = getAllowedBlocksMapArraySize();
    size_t size = map.size();
    setAllowedBlocksMapArraySize(size);
    for (size_t i = 0; i < size; i++)
        setAllowedBlocksMap(i, map[i]);

    // Number of bytes of the map when being serialized
    length += (size - oldSize) * sizeof(uint8_t);
}

const std::vector<CompRbStatus> X2CompProportionalReplyIE::getAllowedBlocksMap() const
{
    size_t size = getAllowedBlocksMapArraySize();
    std::vector<CompRbStatus> map(size);

    for (size_t i = 0; i < size; i++)
        map[i] = getAllowedBlocksMap(i);

    return map;
}

} //namespace
