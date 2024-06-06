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
// User Equipment of LTE stack
//
module LteNicUe extends LteNicBase
{
    parameters:
        LtePdcpRrcType = default("LtePdcpRrcUe");
        LteMacType = default("LteMacUe");
        LtePhyType = default("LtePhyUe");
        packetFlowManager.typename = default("PacketFlowManagerUe");
        d2dCapable = default(false);          // DO NOT CHANGE

    submodules:
        // Feedback generator submodule
        dlFbGen: LteDlFeedbackGenerator {
            @display("p=50,50;is=s");
        }
}

