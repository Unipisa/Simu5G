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


package simu5g.stack.rlc.um;

//
// This module implements the functionalities of the Unacknowledged Mode (UM) of the
// Radio Link Control (RLC) layer for User Equipments supporting Device-to-Device (D2D)
// communications. It extends the basic RLC functionalities with specific management
// functions necessary when a connection is switched from infrastructure mode to D2D mode,
// and vice versa.
//
simple LteRlcUmD2D extends LteRlcUm
{
    parameters:
        @class("LteRlcUmD2D");
        @display("i=block/wheelbarrow");

        //# SDU-level statistics
        @signal[rlcDelayD2D];
        @statistic[rlcDelayD2D](title="Delay at the rlc layer D2D"; unit="s"; source="rlcDelayD2D"; record=mean,vector);
        @signal[rlcThroughputD2D];
        @statistic[rlcThroughputD2D](title="Throughput at the rlc layer D2D"; unit="Bps"; source="rlcThroughputD2D"; record=mean,vector);
        @signal[rlcPacketLossD2D];
        @statistic[rlcPacketLossD2D](title="rlc Packet Loss"; unit=""; source="rlcPacketLossD2D"; record=mean,vector);

        @signal[rlcCellPacketLossD2D];
        @statistic[rlcCellPacketLossD2D](title="rlc Cell Packet Loss"; unit=""; source="rlcCellPacketLossD2D"; record=mean,vector);
        @signal[rlcCellThroughputD2D];
        @statistic[rlcCellThroughputD2D](title="Cell Throughput at the rlc layer D2D"; unit="Bps"; source="rlcCellThroughputD2D"; record=mean,vector);

        //# PDU-level statistics
        @signal[rlcPduDelayD2D];
        @statistic[rlcPduDelayD2D](title="Delay at the rlc layer D2D"; unit="s"; source="rlcPduDelayD2D"; record=mean,vector);
        @signal[rlcPduThroughputD2D];
        @statistic[rlcPduThroughputD2D](title="Throughput at the rlc layer D2D"; unit="Bps"; source="rlcPduThroughputD2D"; record=mean,vector);
        @signal[rlcPduPacketLossD2D];
        @statistic[rlcPduPacketLossD2D](title="rlc Packet Loss"; unit=""; source="rlcPduPacketLossD2D"; record=mean,vector);
}

