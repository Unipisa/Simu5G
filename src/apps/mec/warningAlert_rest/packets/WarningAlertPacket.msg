//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


//
// IceAlert Packet used by UEIceAlertApp MEIceAlertApp and MEIceAlertService
//

import nodes.mec.MEPlatform.MECPackets;

class WarningAppPacket extends MECPacket
{}

class WarningAlertPacket extends WarningAppPacket
{       
    //meaningfull for INFO_UEAPP type  -> used by the MEWarningAlertService
    int ueOmnetID; 
    double positionX;
    double positionY;
    double positionZ;
    
    //meaningfull for INFO_MEAPP type -> used by the UEWarningAlertApp to change its color
    bool danger;
}

class WarningStartPacket extends WarningAppPacket
{       
	double centerPositionX;
	double centerPositionY;
	
	double radius;
}
