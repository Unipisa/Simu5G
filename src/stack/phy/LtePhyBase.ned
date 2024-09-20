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


package simu5g.stack.phy;

//
// Base module for the Physical (PHY) layer of the LTE protocol stack.
//
// This module provides the common functionalities for PHY operations at both the
// eNodeB and User Equipment (UE) sides, such as the initialization of ChannelModel
// module instances, one for each Carrier Component (CC) supported by the node, and
// the transmission of frames over the radio. It provides parameters to configure
// the transmission power and direction of the node, as well as to enable or disable
// handover procedures.
//
simple LtePhyBase like ILtePhy
{
    parameters:
        @display("i=block/wrxtx");
        double ueTxPower @unit(dBm) = default(26dBm);
        double eNodeBTxPower @unit(dBm) = default(46dBm);
        double microTxPower @unit(dBm) = default(30dBm);
        string txDirection @enum(ANISOTROPIC,OMNI) = default("OMNI");
        double txAngle @unit(deg) = default(0deg);

        bool dynamicCellAssociation = default(false);

        // switch for handover messages handling on UEs
        bool enableHandover = default(false);
        double handoverLatency @unit(s) = default(0.05s);

        // TODO move to LtePhyUeD2D module
        bool enableMulticastD2DRangeCheck = default(false);
        double multicastD2DRange @unit(m) = default(1000m);
        string binderModule = default("binder");
        string channelModelModule = default("^.channelModel[0]"); // or "nrChannelModel[0]"

    gates:
        input upperGateIn;       // from upper layer
        output upperGateOut;    // to upper layer
        input radioIn;           // gate to receive message via sendDirect
}

