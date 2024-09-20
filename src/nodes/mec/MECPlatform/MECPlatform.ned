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

package simu5g.nodes.mec.MECPlatform;

import inet.applications.contract.IApp;
import simu5g.nodes.mec.MECPlatform.ServiceRegistry.ServiceRegistry;

//
// This module represents the MEC Platform entity within a ~MECHost, as specified in the
// ETSI MEC architecture (ETSI GS MEC 003). It encompasses the MEC services made available to
// MEC applications instantiated within the MEC Host, along with the ~ServiceRegistry.
// The type and number of available MEC services can be configured.
//
module MECPlatform
{
    parameters:
        @display("bgb=480,187;i=block/cogwheel");

        //# Number of MEC Services available
        int numMecServices = default(0);

    gates:
        inout virtInfr[];     // connection to the virtualization infrastructure

    submodules:

        serviceRegistry: ServiceRegistry {
            @display("p=94,78");
        }

        mecService[numMecServices]: <> like IApp {
            @display("p=320,78,row,140");
        }

    connections allowunconnected:

        serviceRegistry.socketOut --> virtInfr$o[0];
        serviceRegistry.socketIn <-- virtInfr$i[0];

        for i=0..numMecServices-1 {
            mecService[i].socketOut --> virtInfr$o[i+1];
            mecService[i].socketIn <-- virtInfr$i[i+1];
        }
}
