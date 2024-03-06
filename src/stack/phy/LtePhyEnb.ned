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
// eNodeB LtePhy module of PHY Layer
//
simple LtePhyEnb extends LtePhyBase
{
    @class("LtePhyEnb");

    double targetBler = default(0.001);
    double lambdaMinTh = default(0.02);
    double lambdaMaxTh = default(0.2);
    double lambdaRatioTh = default(20);
}

