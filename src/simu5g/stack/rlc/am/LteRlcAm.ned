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


package simu5g.stack.rlc.am;

//
// This module implements the functionalities of the Acknowledged Mode (AM) of the
// Radio Link Control (RLC) layer.
// At the transmitting side, it stores packets coming from the Packet Data Convergence
// Protocol (PDCP) layer into a transmission buffer and, upon notification of a
// transmission opportunity from the Medium Access Control (MAC) layer, sends down
// a RLC Protocol Data Unit (PDU) of the appropriate size (via segmentation and/or
// concatenation).
// At the receiving side, it stores RLC PDUs arriving from the MAC layer in order to
// perform reassembly and in-sequence delivery of packets to the PDCP layer.
// It implements an Automatic Repeat reQuest (ARQ) mechanism to retransmit
// unacknowledged RLC PDUs at the transmitting side and provide guaranteed delivery
// at the receiving side.
//
simple LteRlcAm
{
    parameters:
        @display("i=block/wheelbarrow");

        @signal[rlcDelayDl];
        @statistic[rlcDelayDl](title="Delay at the rlc layer UL"; unit="s"; source="rlcDelayDl"; record=mean);
        @signal[rlcThroughputDl];
        @statistic[rlcThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcThroughputDl"; record=mean);
        @signal[rlcDelayUl];
        @statistic[rlcDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcDelayUl"; record=mean);
        @signal[rlcThroughputUl];
        @statistic[rlcThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcThroughputUl"; record=mean);
        @signal[rlcPduDelayDl];
        @statistic[rlcPduDelayDl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayDl"; record=mean);
        @signal[rlcPduThroughputDl];
        @statistic[rlcPduThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcPduThroughputDl"; record=mean);
        @signal[rlcPduDelayUl];
        @statistic[rlcPduDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayUl"; record=mean);
        @signal[rlcPduThroughputUl];
        @statistic[rlcPduThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcPduThroughputUl"; record=mean);
        @signal[rlcCellThroughputUl];
        @statistic[rlcCellThroughputUl](title="Cell Throughput at the rlc layer UL"; unit="Bps"; source="rlcCellThroughputUl"; record=mean);
        @signal[rlcCellThroughputDl];
        @statistic[rlcCellThroughputDl](title="Cell Throughput at the rlc layer DL"; unit="Bps"; source="rlcCellThroughputDl"; record=mean);
        @signal[rlcCellPacketLossDl];
        @statistic[rlcCellPacketLossDl](title="rlc Cell Packet Loss"; unit=""; source="rlcCellPacketLossDl"; record=mean);
        @signal[rlcCellPacketLossUl];
        @statistic[rlcCellPacketLossUl](title="rlc Cell Packet Loss"; unit=""; source="rlcCellPacketLossUl"; record=mean);
        @signal[rlcPacketLossUl];
        @statistic[rlcPacketLossUl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossUl"; record=mean);
        @signal[rlcPacketLossDl];
        @statistic[rlcPacketLossDl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossDl"; record=mean);
        @signal[rlcPacketLossTotal];
        @statistic[rlcPacketLossTotal](title="rlc Packet Loss"; unit=""; source="rlcPacketLossTotal"; record=mean);
        @signal[rlcPduPacketLossUl];
        @statistic[rlcPduPacketLossUl](title="rlc Packet Loss"; unit=""; source="rlcPduPacketLossUl"; record=mean);
        @signal[rlcPduPacketLossDl];
        @statistic[rlcPduPacketLossDl](title="rlc Packet Loss"; unit=""; source="rlcPduPacketLossDl"; record=mean);

    gates:
        //#
        //# Gates connecting RLC and AM
        //#
        inout AM_Sap_up;    // Gate to PDCP
        inout AM_Sap_down;    // Gate to RLC Mux
}

