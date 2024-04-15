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

namespace simu5g;

//
// Packet used by the video-on-demand application (~VoDUDPServer, ~VoDUDPClient)
//
class VoDPacket extends inet::FieldsChunk {
    int frameSeqNum;
    int frameLength;
    // temporal scalability layer
    int tid;
    // SNR scalability layer
    int qid;
    simtime_t payloadTimestamp;
}
