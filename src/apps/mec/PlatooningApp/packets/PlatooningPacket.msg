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

class PlatooningAppPacket extends MECPacket
{}

class PlatooningJoinPacket extends PlatooningAppPacket
{       
    bool response;    // flag set by the MEC app with the ACK/NACK to join a platoon
    string color;     // color assigned to the members of the platoon
}

class PlatooningLeavePacket extends PlatooningAppPacket
{       
    bool response;    // flag set by the MEC app with the ACK/NACK to leave a platoon

}

class PlatooningInfoPacket extends PlatooningAppPacket
{       
    double newAcceleration;   // new acceleration value to assign to the UE 
}


