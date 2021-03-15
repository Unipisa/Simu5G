// 
//                           Simu5G
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.stack.backgroundTrafficGenerator;

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
   
