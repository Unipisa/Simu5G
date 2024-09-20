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
// This module extends the LTE PHY functionalities on the eNodeB side to support device-to-device (D2D)
// communications. Specifically, it enables the computation of Channel Status Information (CSI) related
// to the D2D link between two UEs, which still needs to be stored at the eNodeB for scheduling purposes.
//
simple LtePhyEnbD2D extends LtePhyEnb
{
    parameters:
        @class("LtePhyEnbD2D");
        bool enableD2DCqiReporting = default(true);
}
