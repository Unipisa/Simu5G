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
// simple module implementing the MEC platform manager (MECPM) entity of a 
// MEC system. It does not follow the ETSI specs, but acts only as a 
// passtrhough between the MEC orchestrator and the MEC host modules
//
// The mecOrchestrator module is used to link the MECPM with the MEC orchestrator

simple MecPlatformManager
{
    parameters:
        string mecOrchestrator;
        @display("i=block/control");

}

