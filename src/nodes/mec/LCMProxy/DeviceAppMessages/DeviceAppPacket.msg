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
//cplusplus {{
//#include "nodes/mec/MEPlatform/MECPackets_m.h";
//}}
//import lte.nodes.mec.MEPlatform.MECPackets.MECPacket;


class DeviceAppPacket extends inet::FieldsChunk {
    unsigned int sno;
    simtime_t timestamp;
    
    //type of message
    string type;
    string operation;
}

class DeviceAppStartPacket extends DeviceAppPacket
{
    string mecAppDId;
	string mecAppName;
	string mecAppProvider;
}


class DeviceAppStopPacket extends DeviceAppPacket
{
}

class DeviceAppStartAckPacket extends DeviceAppPacket
{
    string result;
    string ipAddress;
	int port;
}

class DeviceAppStopAckPacket extends DeviceAppPacket
{
    string result;
}




