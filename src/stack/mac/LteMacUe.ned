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
// User Equipment (UE) Medium Access Control (MAC) layer of Long-Term Evolution (LTE) stack
//
// This module manages the Medium Access Control (MAC) layer functionalities for User Equipment (UE) 
// within the LTE network. It is responsible for essential MAC operations like scheduling, buffer 
// management, and handling uplink/downlink transmissions. This includes interacting with the 
// physical layer (PHY) and Radio Link Control (RLC) layer to ensure efficient data transfer over the 
// LTE network. The LteMacUe module is crucial for supporting various network services and 
// maintaining high-performance communications in the 5G ecosystem.
//
simple LteMacUe extends LteMacBase
{
    parameters:
        @class("LteMacUe");
        string collectorModule = default("");
}

