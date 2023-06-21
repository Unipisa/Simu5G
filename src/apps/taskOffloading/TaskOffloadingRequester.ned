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

package simu5g.apps.taskOffloading;

import inet.applications.contract.IApp;


simple TaskOffloadingRequester like IApp
{
    parameters:
        int localPort = default(-1);
        int destPort = default(3000);
        string destAddress;
        int PacketSize = default(40);
        double sampling_time @unit("s") = default(0.02s);
        
        volatile int numTaskInstructions = default(intuniform(10000,100000));
        
        double startTime @unit("s") = default(0s);
        double finishTime @unit("s") = default(0s);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
               
        @signal[taskOffloadingSentReq];
        @statistic[taskOffloadingSentReq](title="Sent request ID"; unit=""; source="taskOffloadingSentReq"; record=vector);
        
        @signal[taskOffloadingRtt];
        @statistic[taskOffloadingRtt](title="round-trip service time"; unit=""; source="taskOffloadingRtt"; record=mean,vector);
        
        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}
