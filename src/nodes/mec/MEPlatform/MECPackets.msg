//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

class MECPacket extends inet::FieldsChunk {
   
    unsigned int sno;
    simtime_t timestamp;
    //type of message
    string type;
}
