//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.rrc;

//
// RRC Bearer Management — creates and tears down PDCP, RLC and MAC
// entities for data radio bearers.
//
simple BearerManagement
{
    parameters:
        @display("i=block/control");
        @class(BearerManagement);

        string interfaceTableModule;
        string interfaceName = default("cellular");
        string routingTableModule;
        string binderModule = default("binder");

        // Module references for accessing other stack layers
        string pdcpModule = default("^.^.pdcp.entityManager");
        string rlcUmModule = default("^.^.rlc.entityManager");
        string nrRlcUmModule = default("^.^.nrRlc.entityManager");
        string macModule = default("^.^.mac");
        string nrMacModule = default("^.^.nrMac");
}
