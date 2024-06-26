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
// Interface for the Medium Access Control (MAC) layer of LTE Stack.
// 
// The LteMac moduleinterface provides the definition for the MAC layer
// in the LTE network. It serves as an intermediary between the Radio Link
// Control (RLC) layer and the Physical (PHY) layer, facilitating the transmission
// and control of data packets. The MAC layer is responsible for various 
// functions such as scheduling, HARQ (Hybrid Automatic Repeat Request), 
// and traffic prioritization, ensuring efficient and reliable communication 
// in the LTE network.
//
moduleinterface ILteMac
{
    parameters:
        @display("i=block/mac");
        string interfaceTableModule;

    gates:
        input RLC_to_MAC;    // RLC to MAC
        output MAC_to_RLC;    // MAC to RLC
        input PHY_to_MAC;    // PHY to MAC
        output MAC_to_PHY;    // MAC to PHY
}

