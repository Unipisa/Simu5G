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

import simu5g.stack.LteNicEnb;
import simu5g.stack.phy.LtePhy;
import simu5g.stack.mac.LteMac;
import simu5g.stack.rlc.LteRlc;
import simu5g.stack.phy.ChannelModel.LteChannelModelInterface;
import simu5g.stack.phy.feedback.LteDlFeedbackGenerator;
import simu5g.stack.packetFlowManager.PacketFlowManager;

//
// e/gNodeB with NR stack
//
module NRNicEnb extends LteNicEnbD2D
{
    parameters:
        LtePdcpRrcType = default("NRPdcpRrcEnb");
        LteMacType = default("NRMacGnb");
        LteChannelModelType = default("NRChannelModel_3GPP38_901");
        @display("bgb=694,443");
        PacketFlowManagerType = default("NRPacketFlowManagerGnb");
}

