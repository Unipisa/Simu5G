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

package simu5g.stack.dcX2Forwarder;

//
// Performs bridging of data-plane packets between PDCP entities of two eNodeBs
// or gNodeBs when Dual Connectivity is employed. The module directly interfaces
// with the ~LteX2Manager to transfer such packets over the X2 interface.
//
simple DcX2Forwarder like IDcX2Forwarder
{
    parameters:
        @display("i=block/cogwheel");
        @class("DcX2Forwarder");
    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
        input dataIn; // data packets from PDCP to forward via X2
        output tunnelOut; // data packets received from X2 to send to PDCP
}
