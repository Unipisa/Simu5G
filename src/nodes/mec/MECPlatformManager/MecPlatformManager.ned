
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
// This module implements the Mobile Edge Computing (MEC) platform manager (MECPM)
// entity within a MEC system. It serves as a conduit between the MEC orchestrator 
// and MEC host modules. Its primary role is to instantiate and terminate MEC applications 
// based on the orchestrator's requests, managing the lifecycle of these applications 
// and ensuring they are properly integrated within the virtualization infrastructure 
// and service registry.
//
simple MecPlatformManager
{
    parameters:
        string mecOrchestrator;
        string serviceRegistryModule = default("^.mecPlatform.serviceRegistry");
        string vimModule = default("^.vim");
        @display("i=block/control");
}

