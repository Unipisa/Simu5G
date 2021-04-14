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
import simu5g.stack.mac.LteMac;


// 
// NR MAC module of the gNB
//
simple NRMacGnb extends LteMacEnbD2D {
     parameters:
         @class("NRMacGnb");
         amcType = default("NRAmc");

}

// 
// NR MAC module of the UE
//
simple NRMacUe  extends LteMacUeD2D {
     parameters:
         @class("NRMacUe");

}
