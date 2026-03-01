package simu5g.stack.rlc.tm;

//
// Implements the receiving side of the Transparent Mode (TM) of the
// Radio Link Control (RLC) layer for a specific connection.
// Simply forwards received PDUs to the upper layer without
// reassembly or reordering.
//
simple TmRxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");

    gates:
        input in;       // PDUs from lower mux (MAC)
        output out;     // SDUs to upper mux (PDCP)
}
