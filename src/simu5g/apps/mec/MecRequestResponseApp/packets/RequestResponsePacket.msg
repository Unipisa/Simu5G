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


import nodes.mec.MECPlatform.MECPackets;
import inet.networklayer.common.L3Address;
import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace simu5g;

class RequestResponseAppPacket extends inet::FieldsChunk 
{
    int type;
    unsigned int sno;
    
    // request side
    simtime_t requestSentTimestamp;
    
    // response side
    simtime_t requestArrivedTimestamp;
    double processingTime;
    simtime_t serviceResponseTime;
    simtime_t responseSentTimestamp;        
}
