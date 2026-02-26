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
// Bypass RX PDCP entity for Dual Connectivity secondary nodes.
// Receives PDCP PDUs from RLC (UL from UE) and forwards them
// directly to the master node via X2 without any PDCP processing.
//
simple BypassRxPdcpEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        string binderModule = default("binder");
        int headerCompressedSize @unit(B) = default(-1B);    // unused, present for factory compatibility
}
