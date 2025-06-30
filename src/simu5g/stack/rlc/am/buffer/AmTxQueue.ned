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


package simu5g.stack.rlc.am.buffer;

//
// Transmit Buffer of RLC AM
//
simple AmTxQueue
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int fragmentSize @unit(B) = default(30B);        // Size of fragments
        int maxRtx = default(3);
        double pduRtxTimeout @unit(s) = default(2.0s);
        double ctrlPduRtxTimeout @unit(s) = default(2.0s);
        double bufferStatusTimeout @unit(s) = default(2.0s);
        int txWindowSize = default(200);
}

