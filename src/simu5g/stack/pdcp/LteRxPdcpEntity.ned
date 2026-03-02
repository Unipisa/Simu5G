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

package simu5g.stack.pdcp;

//
// Handles the operations related to the LTE PDCP (Packet Data Convergence Protocol)
// layer at the receiving side of a specific connection. It processes incoming PDCP PDUs from the
// RLC layer, performing header decompression before forwarding them to the upper layer protocols.
//
simple LteRxPdcpEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        @signal[receivedPacketFromLowerLayer];
        @signal[pdcpSduReceived];
        @signal[sentPacketToUpperLayer];
        string binderModule = default("binder");
        int headerCompressedSize @unit(B) = default(-1B);    // Header compressed size (bytes) ( -1B = compression disabled

    gates:
        input in;
        input dcIn @loose;
        output out;
}
