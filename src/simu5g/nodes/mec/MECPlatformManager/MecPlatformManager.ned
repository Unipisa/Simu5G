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

package simu5g.nodes.mec.MECPlatformManager;

//
//# MECPlatformManager module
//
// This module implements the MEC platform manager (MECPM) entity within the ~MECHost.
// It serves as an interface between the MEC orchestrator and the MEC host itself. Specifically,
// it manages interactions with the ~VirtualisationInfrastructureManager to instantiate
// and terminate MEC applications upon requests from the MEC orchestrator.
//
simple MecPlatformManager
{
    parameters:
        string mecOrchestrator;
        string serviceRegistryModule = default("^.mecPlatform.serviceRegistry");
        string vimModule = default("^.vim");
        @display("i=block/control");
}

