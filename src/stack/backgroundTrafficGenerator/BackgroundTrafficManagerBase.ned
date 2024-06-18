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

// BackgroundTrafficManagerBase
//
// This is base module for the traffic manager for background UEs
//
simple BackgroundTrafficManagerBase like IBackgroundTrafficManager
{
    parameters:
        @display("i=block/control");
        @class("BackgroundTrafficManager");
        string binderModule = default("binder");
        int numBgUes;
}
