
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

package simu5g.nodes.mec.VirtualisationInfrastructureManager;


//
// VirtualisationInfrastructureManager
//
// Simple Module for handling the availability of virtualised resources on the MEC host.
// From  ETSI GS MEC 003, it is responsible for allocating, managing and releasing virtualised
// resources of the virtualisation infrastructure;
//
simple VirtualisationInfrastructureManager
{
    parameters:
        @display("i=block/control");

        string binderModule = default("binder");
        int mp1Port = default(10021);
        string scheduling @enum(segregation,fair) = default("segregation");

    gates:
        input virtualisationManagerIn;
        output virtualisationManagerOut;
}



