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

package simu5g.stack.handoverManager;

import simu5g.x2.X2User;

//
// Interface for the Handover Manager
//
moduleinterface ILteHandoverManager extends X2User {
    parameters:
        @display("i=block/cogwheel");
}

simple LteHandoverManager like ILteHandoverManager
{
    parameters:
        @display("i=block/cogwheel");
        @class("LteHandoverManager");
        
        bool losslessHandover = default(false);
        
    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
}
