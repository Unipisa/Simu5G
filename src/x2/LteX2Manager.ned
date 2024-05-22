
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
// LteX2Manager
//
// The LteX2Manager module enables communication between LTE eNodeBs using the X2 interface. 
// It facilitates the interaction between X2 user modules and X2Apps, which manage the 
// communication with peer eNodeBs. To leverage the X2 interface in a new module, 
// connect it to the dataPort gate of the LteX2Manager.
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
