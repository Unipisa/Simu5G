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


package simu5g.apps.mec.WarningAlert;

import inet.applications.contract.IApp;


 //
 //  This is a UE app that asks to a Device App to instantiate the MECWarningAlertApp.
 //  After a successful response, it asks to the MEC app to be notified when the car
 //  enters a circle zone described by x,y center position and the radius. When a danger
 //  event arrives, the car colors becomes red.
 //
 //  The event behavior flow of the app is:
 //  1) send create MEC app to the Device App
 //  2.1) ACK --> send coordinates to MEC app
 //  2.2) NACK --> do nothing
 //  3) wait for danger events
 //  4) send delete MEC app to the Device App
 //


simple UEWarningAlertApp like IApp
{
    parameters:       
         @display("i=block/source");    
        
        //connection infos
        int localPort = default(4000);
        int deviceAppPort = default(4000); // ipAddress of the DeviceApp
        string deviceAppAddress; // port of the DeviceApp
                
        int packetSize = default(10);
        
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
        //autoscheduling infos
        double period @unit("s") = default(1s);
        double startTime @unit("s") = default(0s); 
        double stopTime @unit("s") = default(120s);
        
        string mecAppName = default("MECWarningAlertApp"); // but could be MEWarningAlertApp_rest_External
        
        double positionX = default(210);
        double positionY = default(260);
        double positionZ = default(0);
        double radius= default(60);
        
        bool logger = default(false);
        string interfaceTableModule;
                    
    gates:
        output socketOut;
        input socketIn;
}

