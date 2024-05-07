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
// Receive Entity of RLC UM
//
simple UmRxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        string lteRlcUmModule = default("^.um");
        string macModule = default("^.^.mac");  // TODO set to "^.^.nrMac" when parent module name is "nrRlc"
        double timeout @unit(s) = default(1s);            // Timeout for RX Buffer
        int rxWindowSize = default(16);
}

