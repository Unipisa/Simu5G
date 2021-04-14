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

package simu5g.stack.dualConnectivityManager;

import simu5g.x2.X2User;

//
// Interface for the Dual Connectivity Manager
//
moduleinterface IDualConnectivityManager extends X2User {
    parameters:
        @display("i=block/cogwheel");
}

//
// This module is responsible for bridging packets between PDCP layers
// of two e/gNodeBs when dual connectivity is employed
//
simple DualConnectivityManager like IDualConnectivityManager
{
    parameters:
        @display("i=block/cogwheel");
        @class("DualConnectivityManager");
               
    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
}
