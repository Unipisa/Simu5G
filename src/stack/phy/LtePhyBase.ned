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
// Base LtePhy module of PHY Layer
//
simple LtePhyBase like LtePhy
{
    parameters:
        @display("i=block/wrxtx");
        double ueTxPower = default(26);
        double eNodeBTxPower = default(46);
        double microTxPower = default(30);
        string txDirection = default("OMNI");
        double txAngle = default(0);

        bool dynamicCellAssociation = default(false);

        // switch for handover messages handling on UEs
        bool enableHandover = default(false);
        double handoverLatency @unit(s) = default(0.05s);

        // TODO move to LtePhyUeD2D module
        bool enableMulticastD2DRangeCheck = default(false);
        double multicastD2DRange @unit(m) = default(1000m);

    gates:
        input upperGateIn;       // from upper layer
        output upperGateOut;    // to upper layer
        input radioIn;           // gate to receive message via sendDirect
}

