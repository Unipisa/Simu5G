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


simple TaskOffloadingResponder like IApp
{
    parameters:
        int localPort = default(3000);
        int destPort = default(3000);
        string destAddress;
        
        int responseSize = default(40);
        
        bool enableVimComputing @mutable = default(false);
        double serviceComplexity @mutable = default(1); // expressed in fractions of MIPs (the actual number of instructions will be rounded to the closest integer)
        
        double requiredRam @unit(B) = default(10MB);
        double requiredDisk @unit(B) = default(10MB);
        double requiredCpu = default(50);  // MIPS
        
        @signal[taskOffloadingRcvdReq];
        @statistic[taskOffloadingRcvdReq](title="Received request ID"; unit=""; source="taskOffloadingRcvdReq"; record=vector);
        
        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}

