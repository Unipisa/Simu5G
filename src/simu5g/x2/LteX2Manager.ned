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

//
// This module serves as a pluggable interface for submodules of the e/gNodeB's LTE/NR protocol
// stack that implement functionalities requiring the use of the X2 interface for communication
// with peer e/gNodeBs, such as handover, dual connectivity, and Coordinated MultiPoint (CoMP).
// This module simplifies the interaction for such submodules by abstracting the details of X2Apps,
// which are the modules responsible for the actual communication through the X2 interface.
//
simple LteX2Manager
{
    parameters:
        @display("i=block/cogwheel");
        string interfaceTableModule;
        string binderModule = default("binder");

    gates:
        inout dataPort[]; // connection to X2 user modules
        inout x2[] @loose;       // connections to X2App modules
        inout x2Gtp @loose;      // connections to GtpUserX2 module
}
