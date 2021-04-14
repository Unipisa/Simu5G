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

package simu5g.x2;

import inet.applications.sctpapp.SctpClient;

// X2AppClient
//
// This module extends the SctpClient application, by adding
// an output gate to send messages to the X2 Manager
//
simple X2AppClient extends SctpClient
{
    parameters:
        @class("X2AppClient");
        connectPort = default(5000); // it will be overwritten by the initialization procedure
		startTime = default(0s);
    gates:
        output x2ManagerOut @labels(X2Msg);     
}

