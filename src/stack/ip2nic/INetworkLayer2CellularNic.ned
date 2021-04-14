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
package simu5g.stack.ip2nic;

//
// INetworkLayer2CellularNic performs as a bridge between the network layer and the LTE/NR protocol stack
//
moduleinterface INetworkLayer2CellularNic {
    parameters:
        string nodeType;    
        string interfaceTableModule;
        string routingTableModule;
        @display("i=block/layer");
    gates:
        // connection to network layer.
        // These gate names are recognized by the network layer (same as the ones in the INetworkInterface interface) 
        input upperLayerIn;
        output upperLayerOut;

        // connection to the cellular stack
        inout stackNic @labels(IPDatagram);
}

