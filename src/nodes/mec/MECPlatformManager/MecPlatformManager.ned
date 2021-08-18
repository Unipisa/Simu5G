//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.nodes.mec.MECPlatformManager;

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
        @display("i=block/process");

}

