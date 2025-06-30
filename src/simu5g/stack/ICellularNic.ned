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


package simu5g.stack;

import inet.linklayer.contract.IWirelessInterface;


//
// Interface for the 3GPP Stack.
//
moduleinterface ICellularNic extends IWirelessInterface
{
    parameters:
        @display("i=block/ifcard");
        string nodeType;
        //bool d2dCapable;            // inherit the value from the parent module
        @networkInterface;
    gates:
        inout EUTRAN_RRC_Sap;   // Control of RRC Layer
        inout x2[] @loose;     // optional x2 manager
        input nrRadioIn @loose;        // for NR support
}

