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

package simu5g.stack.compManager.compManagerProportional;

import simu5g.stack.compManager.LteCompManagerBase;

//
// LteCompManagerProportional
//
// This module implements a simple CoMP Coordinated Scheduling algorithm.
// A CompManager module inside an eNodeB can be either a master or a slave. 
// Slave nodes send to the master node the amount of RBs required to satisfy
// their users' load. The master node partitions the bandwidth based on these
// requests and sends back the indication of which RBs are available for 
// transmission and which not.
simple LteCompManagerProportional extends LteCompManagerBase
{
    parameters:
        @display("i=block/cogwheel");
        @class("LteCompManagerProportional");
}
