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


package simu5g.apps.mec.Plaas_framework;

import inet.applications.contract.IApp;

//
// This is a UE app that asks to a Device App to obtain the endpoint of a MEC Consumer App.
// It runs on the vehicles and interacts with the PlaaS framework running on the MEC, by sending 
// requests join or leave a platoon and receiving accelaration values from the 
// Platoon Controller App to maintain the desired inter-vehicle distance and speed.
//
simple UEPlatooningApp like IApp
{
    parameters:       
         @display("i=block/source");    
        
        //connection infos
        int localPort = default(4000);
        int deviceAppPort = default(4000); // ipAddress of the DeviceApp
        string deviceAppAddress; // port of the DeviceApp
                
        int joinRequestPacketSize = default(10);
        int leaveRequestPacketSize = default(10);
        
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
        //autoscheduling infos
        double period @unit("s") = default(1s);
        double startTime @unit("s") = default(0s); 
        double stopTime @unit("s") = default(120s);
        
        double joinTime @unit("s") = default(-1s); 
        double leaveTime @unit("s") = default(-1s); 
        
        // the index of the platoon the UE wants to join. It must be < 1000
        // if -1, the MEC will try to find the best platoon 
        int controllerIndex = default(-1);
        
        bool sinusoidal = default(false);
        
        string mecAppName = default("MECPlatooningConsumerApp");
        
        string interfaceTableModule;
        
        // stats
        @signal[cmdLatency];
        @statistic[cmdLatency](title="cmdLatency"; unit="s"; source="cmdLatency"; record=stats, mean, vector); 
        @signal[speed];
        @statistic[speed](title="Vehicle speed"; unit="double"; source="speed"; record=stats, mean, vector);   
        @signal[acceleration];
        @statistic[acceleration](title="Vehicle acceleration"; unit="double"; source="acceleration"; record=stats, mean, vector);    
        @signal[precedingVehicleDistance];
        @statistic[precedingVehicleDistance](title="Vehicle distance"; unit="double"; source="precedingVehicleDistance"; record=stats, mean, vector);
        
        @signal[calculatedPrecedingVehicleDistance];
        @statistic[calculatedPrecedingVehicleDistance](title="Vehicle distance"; unit="double"; source="calculatedPrecedingVehicleDistance"; record=stats, mean, vector);
            
        @signal[gapDistance];
        @statistic[gapDistance](title="Vehicle distance"; unit="double"; source="gapDistance"; record=stats, mean, vector);
            
        @signal[lifeCycleEvents];
        @statistic[lifeCycleEvents](title="Platooning events"; unit="s"; source="lifeCycleEvents"; interpolationmode="none"; record=stats, mean, vector);    
              
    gates:
        output socketOut;
        input socketIn;
}

