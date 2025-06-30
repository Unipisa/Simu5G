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

class RealTimeVideoStreamingAppPacket extends inet::FieldsChunk
{
	int type;	
} 

class StartRealTimeVideoStreamingAppPacket extends RealTimeVideoStreamingAppPacket
{
    int fps;
    string codec;
    int sessionId;
}



class StartSessionRealTimeVideoStreamingAppPacket extends RealTimeVideoStreamingAppPacket
{
    int fps;
    string codec;
    int sessionId;
}

class StopSessionRealTimeVideoStreamingAppPacket extends RealTimeVideoStreamingAppPacket
{
    int sessionId;
}

class RealTimeVideoStreamingAppSegmentHeader extends RealTimeVideoStreamingAppPacket
{
//    chunkLength = 4;
 	int headerLength = 4;
    int payloadLength; // size of the video data
    int frameLength; // length of the frame
    unsigned long frameNumber;
    unsigned long sequenceNumber;
   	int sessionId;
    bool isLastFragment = false; // marker to check if it is the last fragment of a frame
    bool isFragmented = false;   // flag to see if the packet contains a fragment of the frame
    simtime_t payloadTimestamp;
    uint8_t pictureType;   // picture type of the frame the data in this packet belongs to.
   
    int totalFrags;
    int currentFrags;
}
