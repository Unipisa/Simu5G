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
// This module extends the ~LtePhyBase module by providing the specific PHY functions
// on the eNodeB side. It is responsible for checking if a frame received on the air interface
// originates from a User Equipment (UE) connected to the eNB itself, and it provides the
// functions necessary to compute the Channel Status Information (CSI) for the UEs connected to it.
//
simple LtePhyEnb extends LtePhyBase
{
    @class("LtePhyEnb");

    double targetBler = default(0.001);
    double lambdaMinTh = default(0.02);
    double lambdaMaxTh = default(0.2);
    double lambdaRatioTh = default(20);
}

