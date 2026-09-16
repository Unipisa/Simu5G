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
// Downlink half of a DC-secondary PDCP relay (see ~PdcpRelayEntity): receives an
// already-processed PDCP PDU that the master forwarded over X2 and passes it
// straight to the local RLC, doing no PDCP processing of its own.
//
simple PdcpDownlinkRelay
{
    parameters:
        @dynamic(true);
        @display("i=block/layer");
        @signal[sentPacketToLowerLayer];
        @signal[pdcpSduSent];
        string binderModule = default("binder");

    gates:
        input in;    // From the DcMux (DL PDU tunneled from the master)
        output out;  // To the local RLC entity
}
