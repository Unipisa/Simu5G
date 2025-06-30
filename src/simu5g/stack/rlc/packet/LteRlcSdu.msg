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
// This is the RLC message flowing through LTE stack.
// The relevant field is the Main Packet Sequence Number
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

class LteRlcSdu extends inet::FieldsChunk
{
    //@customize(true);
    // TODO: size 0
    chunkLength = inet::B(1); // TODO: should be a tag;
    unsigned int snoMainPacket;                        // ID of packet (sequence number)
    unsigned int lengthMainPacket;
}
