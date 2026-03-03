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

import simu5g.stack.phy.feedback.LteDlFeedbackGenerator;

//
// The LteNicUe module is a network interface that provides LTE connectivity
// within a UE (User Equipment). It implements the data and signaling planes of the LTE stack,
// which includes PDCP, RLC, MAC, and PHY layers. The parameters and submodules
// allow customization of the UE's behavior.
//
module LteNicUe extends LteNicBase
{
    parameters:
        mac.typename = default("LteMacUe");
        phy.typename = default("LtePhyUe");
        handoverPacketHolder.typename = default("HandoverPacketHolderUe");
        packetFlowObserver.typename = default("PacketFlowObserverUe");
        d2dCapable = default(false);          // DO NOT CHANGE

        rrc.hasHandoverController = true;
        rrc.handoverController.isNr = false;

    submodules:
        // Feedback generator submodule
        dlFbGen: LteDlFeedbackGenerator {
            @display("p=80,380;is=s");
        }
}

