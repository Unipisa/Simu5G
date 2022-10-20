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


package simu5g.apps.mec.FLaaS.UEFLApp;

import inet.applications.contract.IApp;

//
//  This is a UE app that asks to a Device App to obtain the endpoint of a MECResponseApp.
//  After a successful response, it ... TBC
//
simple UEFLApp like IApp
{
    parameters:       
         @display("i=block/source");    
        
        //connection infos
        int localPort = default(4000);
        int deviceAppPort = default(4000); // ipAddress of the DeviceApp
        string deviceAppAddress; // port of the DeviceApp
                
        int requestPacketSize = default(10);
        
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
        //autoscheduling infos
        double period @unit("s") = default(0.2s);
        double startTime @unit("s") = default(0s); 
        double stopTime @unit("s") = default(120s);
                
        string mecAppName = default("MecFLLocalManagerApp");
        
        string interfaceTableModule;         
             
    gates:
        output socketOut;
        input socketIn;
}

