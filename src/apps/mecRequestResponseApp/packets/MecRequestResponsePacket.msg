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
// Packet used by the ~MecRequestRequestPacket and ~MecRequestResponsePacket modules.
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

class MecRequestResponsePacket extends inet::FieldsChunk 
{
    unsigned int sno;
     
    double reqTimestamp;
    double respTimestamp;

    int bsId;
    int appId;
    
    string srcAddress;
    int srcPort;
    string destAddress;
    int destPort;
}
