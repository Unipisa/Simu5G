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
// This module can be used to simulate a background application running on the
// MEC host without simulating all message exchanges and finite-state machines.
// It can be useful for simulating background tasks that utilize CPU, RAM, and disk
// resources.
//
module BackgroundApp
{
    double ram @unit("B") = default(10MB);
    double cpu = default(500);
    double disk @unit("B") = default(10MB);
}
