//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.x2;

//
// Serves as a pluggable interface for submodules of the e/gNodeB's LTE/NR protocol
// stack that implement functionalities requiring the use of the X2 interface for communication
// with peer e/gNodeBs, such as handover, dual connectivity, and Coordinated MultiPoint (CoMP).
// Simplifies the interaction for such submodules by abstracting the details of X2Apps,
// which are the modules responsible for the actual communication through the X2 interface.
//
simple LteX2Manager
{
    parameters:
        @display("i=block/cogwheel");
        string interfaceTableModule;
        string binderModule = default("binder");

    gates:
        input dataPortIn[];  // incoming connections from X2 user modules
        output dataPortOut[]; // outgoing connections to X2 user modules
        input x2In[] @loose;        // incoming connections from X2App modules
        output x2Out[] @loose;      // outgoing connections to X2App modules
        input x2GtpIn @loose;       // incoming connection from GtpUserX2 module
        output x2GtpOut @loose;     // outgoing connection to GtpUserX2 module
}
