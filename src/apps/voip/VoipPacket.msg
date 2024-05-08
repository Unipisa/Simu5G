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
// Packet used by the ~VoPSender and ~VoIPReceiver modules.
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace simu5g;

class VoipPacket extends inet::FieldsChunk {
    unsigned int IDtalk;
    unsigned int nframes;
    unsigned int IDframe;
    simtime_t arrivalTime;
    simtime_t playoutTime;
    simtime_t payloadTimestamp;
    unsigned int payloadSize;
}
