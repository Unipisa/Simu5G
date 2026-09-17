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

import simu5g.stack.NrNicUe;

//
// Device-to-Device (D2D) capable NR User Equipment network interface. It extends
// ~NrNicUe by swapping in the D2D-capable NR MAC/PHY leaves and enabling the D2D
// mode controller, allowing direct UE-to-UE communication without routing traffic
// through the gNodeB. It is the NR counterpart of ~LteNicUeD2D.
//
// The PDCP and RLC entities are missing from the submodules on purpose: the RRC
// layer's ~BearerManagement creates them per bearer at run time.
//
module NrNicUeD2D extends NrNicUe
{
    parameters:
        // NR stack: use the D2D-capable MAC/PHY leaves
        nrMac.typename = default("NrMacUeD2D");
        nrPhy.typename = default("PhyUeD2D");

        // LTE leg: also runs the D2D-capable MAC/PHY leaves (the PHY carries
        // the averageCqiD2D statistic)
        mac.typename = default("LteMacUeD2D");
        phy.typename = default("PhyUeD2D");
        ip2nic.typename = default("Ip2NicD2D");

        // Both legs previously ran with the NIC-wide D2D support enabled
        rlcMux.typename = default("RlcMuxD2D");
        rrc.bearerManagement.lteRlcUmEntityModuleType = default("simu5g.stack.d2d.rlc.LteRlcUmEntityD2D");
        rrc.bearerManagement.nrRlcUmEntityModuleType = default("simu5g.stack.d2d.rlc.NrRlcUmEntityD2D");
        nrRlcMux.typename = default("RlcMuxD2D");

        // Use the D2D-capable channel model on both legs (matching the
        // pre-separation defaults: TR 36.814 on the LTE leg, TR 38.901 on the NR leg)
        channelModelType = default("D2dChannelModel");
        nrChannelModelType = default("D2dChannelModel");
        nrChannelModel[*].pathLossType = default("Tr38901");

        rrc.typename = default("RrcD2D");
        rrc.handoverController.typename = default("HandoverControllerD2D");
        rrc.nrHandoverController.typename = default("HandoverControllerD2D");
        bool d2dInitialMode = default(false);
}
