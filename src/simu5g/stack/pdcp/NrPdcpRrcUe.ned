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
// The NrPdcpRrcUe module implements the PDCP/RRC layer for an NR NIC employed at
// a User Equipment. It extends the ~LtePdcpRrcUeD2D module to enable Dual Connectivity
// and Split Bearer functionalities at the UE.
//
simple NrPdcpRrcUe extends LtePdcpRrcUeD2D
{
    parameters:
        @class("NrPdcpRrcUe");
}

