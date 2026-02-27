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
// Bypass TX PDCP entity for Dual Connectivity secondary nodes.
// Receives already-processed PDCP PDUs (from master via X2) and forwards
// them directly to RLC without any PDCP processing.
//
simple BypassTxPdcpEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        @signal[sentPacketToLowerLayer];
        @signal[pdcpSduSent];
        string binderModule = default("binder");
}
