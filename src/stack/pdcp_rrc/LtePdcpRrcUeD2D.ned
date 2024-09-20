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
// The LtePdcpRrcUeD2D module implements the PDCP/RRC layer for an LTE NIC employed at
// a User Equipment with support for Device-to-Device (D2D) communications. It extends
// the basic PDCP/RRC functionality with D2D communication capabilities.
//
simple LtePdcpRrcUeD2D extends LtePdcpRrcUe
{
    parameters:
        @class("LtePdcpRrcUeD2D");
}

