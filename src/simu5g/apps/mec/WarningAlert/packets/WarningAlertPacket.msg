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
// IceAlert Packet used by UEIceAlertApp MEIceAlertApp and MEIceAlertService
//

import nodes.mec.MECPlatform.MECPackets;

namespace simu5g;

class WarningAppPacket extends MECPacket
{
}

class WarningAlertPacket extends WarningAppPacket
{       
    int ueOmnetID; 
    double positionX;
    double positionY;
    double positionZ;
    bool danger;
}

class WarningStartPacket extends WarningAppPacket
{       
	double centerPositionX;
	double centerPositionY;
	double centerPositionZ;
	
	double radius;
}

class WarningStopPacket extends WarningAppPacket
{       
}
