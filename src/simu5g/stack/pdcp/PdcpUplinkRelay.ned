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
// Uplink half of a DC-secondary PDCP relay (see ~PdcpRelayEntity): receives a PDCP
// PDU from the local RLC (UL from the UE) and forwards it to the master node
// over X2, doing no PDCP processing of its own.
//
simple PdcpUplinkRelay
{
    parameters:
        @dynamic(true);
        @display("i=block/layer");
        string binderModule = default("binder");

    gates:
        input in;    // From the local RLC entity (UL PDU from the UE)
        output out;  // To the DcMux (tunneled to the master over X2)
}
