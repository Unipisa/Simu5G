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
// The LtePdcpRrcEnbD2D module implements the PDCP/RRC layer for a LTE NIC employed at
// eNodeBs, providing support for D2D (Device-to-Device) communications. This module extends
// the basic LtePdcpRrcEnb by adding functionality for managing D2D-capable user equipment,
// specifically for buffering packets during D2D mode switching operations.
//
simple LtePdcpRrcEnbD2D extends LtePdcpRrcEnb
{
    @class("LtePdcpRrcEnbD2D");
}

