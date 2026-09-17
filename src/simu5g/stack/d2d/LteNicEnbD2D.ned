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


package simu5g.stack.d2d;

import simu5g.stack.LteNicEnb;

//
// The LteNicEnbD2D module represents an LTE eNodeB network interface with
// support for D2D (Device-to-Device) communications. It extends the basic
// LteNicEnb module to manage D2D-capable UEs (User Equipments). A D2D mode
// selection algorithm can be specified via the rrc.d2dModeSelection
// submodule's typename.
//
// The PDCP and RLC entities are missing from the submodules on purpose: the RRC
// layer's ~BearerManagement creates them per bearer at run time.
//
module LteNicEnbD2D extends LteNicEnb
{
    parameters:
        mac.typename = default("LteMacEnbD2D");
        phy.typename = default("PhyEnbD2D");
        packetFlowObserver.typename = default("PacketFlowObserverEnb");
        ip2nic.typename = default("Ip2NicD2D");
        rlcMux.typename = default("RlcMuxD2D");
        rrc.bearerManagement.lteRlcUmEntityModuleType = default("simu5g.stack.d2d.rlc.LteRlcUmEntityD2D");

        // Use the D2D-capable channel model
        channelModelType = default("D2dChannelModel");
        rrc.typename = default("RrcD2D");
        rrc.hasD2DModeSelection = true;
}
