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

import simu5g.stack.LteNicUe;


//
// Represents a Device-to-Device (D2D) capable User Equipment (UE)
// network interface. It extends the basic LTE UE functionality by adding D2D
// communication capabilities, allowing direct communication between UEs without
// the need to route traffic through the base station.
//
// The PDCP and RLC entities are missing from the submodules on purpose: the RRC
// layer's ~BearerManagement creates them per bearer at run time.
//
module LteNicUeD2D extends LteNicUe
{
    parameters:
        mac.typename = default("LteMacUeD2D");
        phy.typename = default("PhyUeD2D");
        packetFlowObserver.typename = default("PacketFlowObserverUe");
        ip2nic.typename = default("Ip2NicD2D");
        rlcMux.typename = default("RlcMuxD2D");
        rrc.bearerManagement.lteRlcUmEntityModuleType = default("simu5g.stack.d2d.rlc.LteRlcUmEntityD2D");

        // Use the D2D-capable channel model
        channelModelType = default("D2dChannelModel");

        rrc.typename = default("RrcD2D");
        rrc.handoverController.typename = default("HandoverControllerD2D");
        bool d2dInitialMode = default(false);
}

