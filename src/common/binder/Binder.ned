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


package simu5g.common.binder;

// 
// Binder module
//
// It is used to store any information related to 
// the network that needs to be accessed via method calls, e.g. 
// for control-plane functionalities.
// There must be one (and one only) instance of the module in the network
//
simple Binder 
{
    parameters:
        int blerShift = default(0);
        
        @display("i=block/cogwheel");
}
