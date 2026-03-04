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


package simu5g.stack;

import simu5g.stack.sdap.INrSdap;

//
// The NrNicEnb module serves as a network interface that provides 5G connectivity
// within a gNodeB. It implements the data and signaling plane of the NR (New Radio) stack,
// which encompasses the PDCP, RLC, MAC, and PHY layers. The parameters and submodules
// facilitate the customization of the gNodeB's behavior, including its support for
// D2D communication, CoMP (Coordinated MultiPoint), or dual connectivity.
//
module NrNicEnb extends LteNicEnbD2D
{
    parameters:
        bool hasSdap = default(false);
        ip2nic.hasSdap = default(this.hasSdap);
        sdap.nodeRole = default("gNB");
        pdcpMux.isNR = default(true);
        rrc.bearerManagement.pdcpRxEntityModuleType = default("simu5g.stack.pdcp.NrRxPdcpEntity");
        rrc.bearerManagement.pdcpTxEntityModuleType = default("simu5g.stack.pdcp.NrTxPdcpEntity");
        mac.typename = default("NrMacGnb");
        lteChannelModelType = default("NrChannelModel_3GPP38_901");
        packetFlowObserver.typename = default("NrPacketFlowObserverGnb");

    submodules:
        sdap: <default("NrSdap")> like INrSdap if hasSdap {
            @display("p=230,155");
        }

    connections:
        ip2nic.stackOut --> { @reconnect; } --> sdap.upperLayerIn if hasSdap;
        sdap.pdcpOut --> pdcpMux.upperLayerIn if hasSdap;
        pdcpMux.upperLayerOut --> { @reconnect; } --> sdap.pdcpIn if hasSdap;
        sdap.upperLayerOut --> ip2nic.stackIn if hasSdap;
}

