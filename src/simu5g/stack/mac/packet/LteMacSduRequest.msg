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

import simu5g.common.LteCommon;

namespace simu5g;

//
// This is the message that asks the RLC layer for a new MAC SDU.
//
class LteMacSduRequest extends inet::FieldsChunk
{
    unsigned int sduSize;       // size of the requested SDU
    chunkLength = inet::B(1); // TODO: size 0
    MacNodeId ueId = NODEID_NONE;     // ID of the destination UE associated with the request
    LogicalCid lcid = 0;              // Logical Connection identifier associated with the request
}
