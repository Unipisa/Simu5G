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
// D2D-capable User Equipment LtePhy module of PHY Layer
//
simple LtePhyUeD2D extends LtePhyUe
{
    parameters:
        @class("LtePhyUeD2D");
        double d2dTxPower = default(26);
        bool d2dMulticastCaptureEffect = default(true);
        string d2dMulticastCaptureEffectFactor = default("RSRP");  // or distance

        //# D2D CQI statistic
        @signal[averageCqiD2D];
        @statistic[averageCqiD2D](title="Average Cqi reported in D2D"; unit="cqi"; source="averageCqiD2D"; record=mean,vector);
}

