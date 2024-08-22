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
import inet.common.geometry.Geometry;
import common.LteCommon;

cplusplus {{
const inet::B D2D_MULTIHOP_HEADER_LENGTH = inet::B(8);
}}

namespace simu5g;

//
// MultihopD2DPacket
//
// Extend this packet for creating your own multihop message
//
class MultihopD2DPacket extends inet::FieldsChunk
{
    MacNodeId srcId;
    uint32_t msgid;          // unique id of the message  < sender id  | local msg id >

    inet::Coord srcCoord;
    double maxRadius;

    int ttl;                 // time-to-live (to avoid flooding)
    unsigned int hops;       // number of hops

    MacNodeId lastHopSenderId;

    simtime_t payloadTimestamp;
    unsigned int payloadSize;

    chunkLength = D2D_MULTIHOP_HEADER_LENGTH;
}
