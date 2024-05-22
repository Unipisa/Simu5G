
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
// This module represents a User Equipment (UE) device implementing a 
// Long Term Evolution (LTE) stack. It includes various protocol layers 
// like PDCP, RLC, MAC, and PHY.
//
// Parameters:
// - LtePdcpRrcType: Specifies the type of PDCP and RRC stack to use.
// - LteMacType: Specifies the type of MAC stack to use.
// - LtePhyType: Specifies the type of PHY stack to use.
// - PacketFlowManagerType: Specifies the type of packet flow manager.
// - d2dCapable: Indicates if the UE is capable of Device-to-Device (D2D) communication.
//
// Submodules:
// - dlFbGen: Downlink Feedback Generator for managing downlink feedback.
//
module LteNicUe extends LteNicBase
{
    parameters:
        pdcpRrc.typename = default("LtePdcpRrcUe");
        mac.typename = default("LteMacUe");
        phy.typename = default("LtePhyUe");
        packetFlowManager.typename = default("PacketFlowManagerUe");
        d2dCapable = default(false);          // DO NOT CHANGE

    submodules:
        // Feedback generator submodule
        dlFbGen: LteDlFeedbackGenerator {
            @display("p=50,50;is=s");
        }
}

