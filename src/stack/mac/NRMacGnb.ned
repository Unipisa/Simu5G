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
// NR MAC module of the gNB
//
// Medium Access Control (MAC) layer of the gNodeB (gNB) for New Radio (NR) communication.
// This module is responsible for managing the MAC layer functionalities, including 
// scheduling, handling of Hybrid Automatic Repeat Request (HARQ) processes, and 
// resource allocation. It plays a crucial role in the 5G network by ensuring efficient 
// data transmission and reception, adapting to varying network conditions, and 
// supporting enhanced mobile broadband services.
//
simple NRMacGnb extends LteMacEnbD2D
{
    parameters:
        @class("NRMacGnb");
        amcType = default("NRAmc");
}

