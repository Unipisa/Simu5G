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

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import common.LteCommon;

namespace simu5g;

//
// This is the X2 message flowing through X2 interface.
// It contains a vector of Information Elements (see LteX2Message.h),
// the total length of the message and the type of the message
//
class LteX2Message extends inet::FieldsChunk
{
    @customize(true);
    chunkLength = inet::B(11); // FIXME needs to be checked:
                               // realistic chunk length without X2 information elements?
                               // Must at least be 11 Bytes for serialization:
                               // Type (1 B), sourceId (4 B), destinationId (4 B), IE list size (2 B))
    MacNodeId sourceId;
    MacNodeId destinationId;
}
