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
// This module handles operations related to the LTE PDCP (Packet Data Convergence Protocol)
// layer on the transmitting side of a specific connection. This module manages the creation,
// sequencing, header compression, and metadata association for PDCP PDUs before they are passed
// down to the RLC layer.
//
simple LteTxPdcpEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
}

