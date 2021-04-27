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


package simu5g.stack.backgroundTrafficGenerator;

moduleinterface IBackgroundTrafficManager 
{
    parameters:
        @display("i=block/control");
}

// BackgroundTrafficManager
//
// This is the traffic manager for background UEs within a
// real (simulated) cell. It interacts with the protocol
// stack of the e/gNB
simple BackgroundTrafficManager like IBackgroundTrafficManager
{
    parameters:
        @display("i=block/control");
        @class("BackgroundTrafficManager");
}    
   
