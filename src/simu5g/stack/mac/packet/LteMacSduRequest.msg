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

//
// This is the message that asks the RLC layer for a new MAC SDU.
//
import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace simu5g;

class LteMacSduRequest extends inet::FieldsChunk
{
    @customize(true);
    unsigned int sduSize;       // size of the requested SDU
    chunkLength = inet::B(1); // TODO: size 0
}
