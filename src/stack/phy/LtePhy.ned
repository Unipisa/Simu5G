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
// Interface for the LtePhy module of PHY Layer
//
moduleinterface LtePhy {
    parameters:
        @display("i=block/wrxtx");
        
    gates:
        input upperGateIn;
        output upperGateOut;
        input radioIn;
}

// 
// Base LtePhy module of PHY Layer
//
simple LtePhyBase like LtePhy {
    parameters:
        @display("i=block/wrxtx");
        double ueTxPower =default(26);
        double eNodeBTxPower =default(46);
        double microTxPower =default(30);
        string txDirection = default("OMNI");
        double txAngle = default(0);       
        
        bool dynamicCellAssociation = default(false);
        
        // switch for handover messages handling on UEs
        bool enableHandover = default(false);
        double handoverLatency @unit(s) = default(0.05s);
        
        // TODO move to LtePhyUeD2D module
        bool enableMulticastD2DRangeCheck = default(false);
        double multicastD2DRange @unit(m) = default(1000m);
               
    gates:
        input upperGateIn;       // from upper layer
        output upperGateOut;    // to upper layer
        input radioIn;           // gate to receive message via sendDirect
}

// 
// User Equipment LtePhy module of PHY Layer
//
simple LtePhyUe extends LtePhyBase {
     parameters:
         @class("LtePhyUe");
         
         // if true, get minRssi from PhyPisaData
         bool minRssiDefault = default(true);
         
         // configurable minimum threshold RSSI for attaching to an eNB (meaningful only if minRssiDefault==false)
         double minRssi @unit("dB")= default(-99.0dB);
         
         @signal[servingCell];
         @statistic[servingCell](title="ID of the serving eNodeB for the UE"; unit=""; source="servingCell"; record=vector);
         
         //# CQI statistics
         @signal[averageCqiDl];
         @statistic[averageCqiDl](title="Average Cqi reported in DL"; unit="cqi"; source="averageCqiDl"; record=mean,vector);
         @signal[averageCqiUl];
         @statistic[averageCqiUl](title="Average Cqi reported in UL"; unit="cqi"; source="averageCqiUl"; record=mean,vector);
}

// 
// D2D-capable User Equipment LtePhy module of PHY Layer
//
simple LtePhyUeD2D extends LtePhyUe {
     parameters:
         @class("LtePhyUeD2D");
         double d2dTxPower =default(26);
         bool d2dMulticastCaptureEffect = default(true);
         string d2dMulticastCaptureEffectFactor = default("RSRP");  // or distance
         
         //# D2D CQI statistic
         @signal[averageCqiD2D];
         @statistic[averageCqiD2D](title="Average Cqi reported in D2D"; unit="cqi"; source="averageCqiD2D"; record=mean,vector);
}

// 
// eNodeB LtePhy module of PHY Layer
//
simple LtePhyEnb extends LtePhyBase {
    @class("LtePhyEnb");
    
    double targetBler = default(0.001);
    double lambdaMinTh = default(0.02);
    double lambdaMaxTh = default(0.2);
    double lambdaRatioTh = default(20);
}

// 
// eNodeB LtePhy module of PHY Layer with support for D2D
//
simple LtePhyEnbD2D extends LtePhyEnb {
    parameters:
        @class("LtePhyEnbD2D");
        bool enableD2DCqiReporting = default(true);
}
