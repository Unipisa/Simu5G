
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

package simu5g.stack.pdcp_rrc;

//
// This module represents the Transmit Entity of NR (New Radio) PDCP (Packet Data Convergence Protocol).
// It extends the functionalities of the LteTxPdcpEntity and incorporates additional features 
// necessary for NR transmissions.
//
// @neddoc[Entity responsible for transmitting PDCP PDUs in the LTE stack. 
// This module handles the creation, sequencing, and metadata association 
// for PDCP PDUs before they are passed down to the RLC layer. 
// It includes error-checking, header compression, and sequence number management.]
//
simple LteTxPdcpEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
}

