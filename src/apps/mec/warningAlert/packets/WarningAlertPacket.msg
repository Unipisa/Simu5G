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

import nodes.mec.MEPlatform.MEAppPacket;

class WarningAlertPacket extends MEAppPacket
{       
    //meaningfull for INFO_UEAPP type  -> used by the MEWarningAlertService
    int ueOmnetID; 
    double positionX;
    double positionY;
    double positionZ;
    
    //meaningfull for INFO_MEAPP type -> used by the UEWarningAlertApp to change its color
    bool danger;
}
