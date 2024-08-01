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


package simu5g.stack.rlc;

//
// Transmit Entity of RLC UM
//
simple UmTxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        string packetFlowManagerModule = default("");
        string umModule = default("^.um");
        int fragmentSize @unit(B) = default(30B);        // Size of fragments
}

