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

package simu5g.apps.mecRequestResponseApp;

import inet.applications.contract.IApp;

simple MecRequestApp like IApp
{
    parameters:       
         @display("i=block/source");    
        
        int localPort = default(4000);
        int destPort = default(4000);
        string destAddress;
        volatile int packetSize = intuniform(8000,12000);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
        double period @unit("s") = default(1s);
        double startTime @unit("s") = default(0s); 
        double stopTime @unit("s") = default(200s);
        
        int appId = default(1);
        bool enableMigration = default(false);
                     
        @signal[requestSize];
        @statistic[requestSize](title="Request packet size"; unit=""; source="requestSize"; record=stats, vector);   
        @signal[requestRTT];
        @statistic[requestRTT](title="Round trip time"; unit=""; source="requestRTT"; record=stats, vector);    
        @signal[recvResponseSno];
        @statistic[recvResponseSno](title="Received Sequence number"; unit=""; source="recvResponseSno"; record=vector);    
    gates:
        
        output socketOut;
        input socketIn;
}

