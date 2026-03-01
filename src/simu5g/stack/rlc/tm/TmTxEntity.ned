package simu5g.stack.rlc.tm;

//
// Implements the transmission side of the Transparent Mode (TM) of the
// Radio Link Control (RLC) layer for a specific connection.
// Buffers SDUs from the upper layer and, upon receiving a MAC SDU request,
// sends the next queued PDU without segmentation or concatenation.
//
simple TmTxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int queueSize = default(0);    // max queue length in packets (0: unlimited)

        @signal[rlcPacketLossDl];
        @statistic[rlcPacketLossDl](title="rlc Packet Loss DL"; unit=""; source="rlcPacketLossDl"; record=mean,sum,vector);
        @signal[rlcPacketLossUl];
        @statistic[rlcPacketLossUl](title="rlc Packet Loss UL"; unit=""; source="rlcPacketLossUl"; record=mean,sum,vector);

    gates:
        input in;       // SDUs from upper mux (PDCP)
        input macIn;    // SDU requests from lower mux (MAC)
        output out;     // PDUs and new-data notifications to lower mux (MAC)
}
