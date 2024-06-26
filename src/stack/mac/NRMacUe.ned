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

package simu5g.stack.mac;
//
// New Radio (NR) Medium Access Control (MAC) module for the User Equipment (UE)
// This module implements the NR MAC layer functionalities specific to the User Equipment (UE)
// in a 5G network. It extends the capabilities of LteMacUeD2D by supporting new NR-specific 
// features and protocols, ensuring efficient data transmission, resource allocation, and 
// overall communication performance in NR systems.
//
simple NRMacUe extends LteMacUeD2D
{
    parameters:
        @class("NRMacUe");
}
