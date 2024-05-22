
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

//
// The BackgroundTrafficManager is responsible for overseeing background traffic
// management for UEs within a simulated cell. It ensures the effective handling
// of background traffic by coordinating with various components of the e/gNB's 
// protocol stack. This module supports assigning resource blocks, managing 
// interference, and updating CQIs for background UEs, thus playing a crucial role 
// in simulating realistic traffic conditions in LTE/5G networks.
//
simple BackgroundTrafficManager extends BackgroundTrafficManagerBase
{
    parameters:
        @display("i=block/control");
        @class("BackgroundTrafficManager");
        string macModule = default("^.^.mac");
        string phyModule = default("^.^.phy");
        bool enablePeriodicCqiUpdate;
        bool computeAvgInterference;
}

