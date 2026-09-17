//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
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
// CoMP (Coordinated MultiPoint) or dual connectivity. D2D support is added by the
// ~NrNicEnbD2D subtype.
//
// The PDCP and RLC entities are missing from the submodules on purpose: the RRC
// layer's ~BearerManagement creates them per bearer at run time.
//
module NrNicEnb extends LteNicEnb
{
    parameters:
        bool hasSdap = default(false);  // Whether the stack has an SDAP layer: true on 5GC (Upf) networks, false on EPC/EN-DC ones; the network sets it per its core
        ip2nic.hasSdap = default(this.hasSdap);
        sdap.nodeRole = default("gNB");
        ip2nic.isNr = default(true);
        phy.isNr = default(true);
        rrc.bearerManagement.pdcpEntityModuleType = default("simu5g.stack.pdcp.NrPdcpEntity");
        mac.typename = default("NrMacGnb");
        channelModelType = default("Tr38901ChannelModel");
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

