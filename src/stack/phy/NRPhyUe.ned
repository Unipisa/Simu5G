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


package simu5g.stack.phy;

//
// This module provides the functionalities for the Physical (PHY) layer of the
// NewRadio (NR) protocol stack at the User Equipment (UE) side. In particular, 
// it takes care of handling transmission/reception of frames over the radio and 
// handover procedures when using the NR technology.
//
simple NRPhyUe extends LtePhyUeD2D
{
    parameters:
        @class("NRPhyUe");
        string otherPhyModule;
}

