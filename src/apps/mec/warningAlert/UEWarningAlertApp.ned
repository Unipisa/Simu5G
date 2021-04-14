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

package simu5g.apps.mec.warningAlert;

import inet.applications.contract.IApp;

// UEWarningAlertApp simple module in charge to:
//
// 1) sending WarningAlertPacket of type START_MEAPP until receiving an ACK_START_MEAPP type 
//		to instantiate the required MEWarningAlertAlertApp instance
// 2) handling ACK_START_MEAPP
// 3) sending WarningAlertPacket of type INFO_UEAPP with its informations (position)
// 4) handling WarningAlertPacket of type INFO_MEAPP alerting of Danger on the road
// 5) sending WarningAlertPacket of type STOP_MEAPP until receiving an ACK_STOP_MEAPP type
//		to terminate the MEWarningAlertApp instance
// 6) handlin ACK_STOP_MEAPP

simple UEWarningAlertApp like IApp
{
    parameters:       
         @display("i=block/source");    
        
        //connection infos
        int localPort = default(4000);
        int destPort = default(4000);
        string destAddress;
        int packetSize = default(10);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
        //resource required infos
        int requiredRam @unit("B") = default(10MB);
        int requiredDisk @unit("B") = default(100MB);
        double requiredCpu = default(0.01);
        
        //autoscheduling infos
        double period @unit("s") = default(1s);
        double startTime @unit("s") = default(0s); 
        double stopTime @unit("s") = default(120s);
        
        string interfaceTableModule;
                    
    gates:
        
        output socketOut;
        input socketIn;
}

