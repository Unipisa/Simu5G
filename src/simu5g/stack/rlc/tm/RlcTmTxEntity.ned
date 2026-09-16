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

package simu5g.stack.rlc.tm;

//
// Implements the transmission side of the Transparent Mode (TM) of the
// Radio Link Control (RLC) layer for a specific connection.
// Buffers SDUs from the upper layer and, upon receiving a MAC SDU request,
// sends the next queued PDU without segmentation or concatenation.
//
simple RlcTmTxEntity like simu5g.stack.rlc.IRlcTxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/layer");
        int queueSize = default(0);    // Max queue length in packets (0: unlimited)

        @signal[rlcPacketLossDl];
        @statistic[rlcPacketLossDl](title="rlc Packet Loss DL"; unit=""; source="rlcPacketLossDl"; record=mean,sum,vector);
        @signal[rlcPacketLossUl];
        @statistic[rlcPacketLossUl](title="rlc Packet Loss UL"; unit=""; source="rlcPacketLossUl"; record=mean,sum,vector);

    gates:
        input in;       // SDUs from the PDCP layer (direct per-DRB connection, not via a mux)
        input macIn;    // MAC SDU requests, via the RlcMux
        output out;     // PDUs and new-data notifications to MAC, via the RlcMux
}
