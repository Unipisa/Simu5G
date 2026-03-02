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

        // PDCP entity parameters
        int headerCompressedSize @unit(B) = default(-1B);

        // PDCP entity types
        string pdcpRxEntityModuleType = default("simu5g.stack.pdcp.LteRxPdcpEntity");
        string pdcpTxEntityModuleType = default("simu5g.stack.pdcp.LteTxPdcpEntity");
        string pdcpBypassRxEntityModuleType = default("simu5g.stack.pdcp.BypassRxPdcpEntity");
        string pdcpBypassTxEntityModuleType = default("simu5g.stack.pdcp.BypassTxPdcpEntity");

        // RLC entity types
        string rlcUmTxEntityModuleType = default("simu5g.stack.rlc.um.UmTxEntity");
        string rlcUmRxEntityModuleType = default("simu5g.stack.rlc.um.UmRxEntity");
        string rlcTmTxEntityModuleType = default("simu5g.stack.rlc.tm.TmTxEntity");
        string rlcTmRxEntityModuleType = default("simu5g.stack.rlc.tm.TmRxEntity");
        string rlcAmTxEntityModuleType = default("simu5g.stack.rlc.am.AmTxQueue");
        string rlcAmRxEntityModuleType = default("simu5g.stack.rlc.am.AmRxQueue");

        // Module references for accessing other stack layers
        string pdcpModule = default("^.^.pdcp.entityManager");
        string rlcUmModule = default("^.^.rlc.entityManager");
        string nrRlcUmModule = default("^.^.nrRlc.entityManager");
        string macModule = default("^.^.mac");
        string nrMacModule = default("^.^.nrMac");
}
