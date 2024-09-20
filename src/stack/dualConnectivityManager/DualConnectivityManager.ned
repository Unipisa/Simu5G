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

package simu5g.stack.dualConnectivityManager;

//
// This module is responsible for bridging data-plane packets between PDCP entities
// of two eNodeBs or gNodeBs when Dual Connectivity is employed. The module directly interfaces
// with the ~LteX2Manager to transfer such packets over the X2 interface.
//
simple DualConnectivityManager like IDualConnectivityManager
{
    parameters:
        @display("i=block/cogwheel");
        @class("DualConnectivityManager");
        string pdcpRrcModule = default("^.pdcpRrc");

    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
}
