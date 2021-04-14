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
// Interface for the MAC layer of LTE Stack.
//
moduleinterface LteMac {
    parameters:
        @display("i=block/mac");
        string interfaceTableModule;
   
    gates:
        input RLC_to_MAC;    // RLC to MAC
        output MAC_to_RLC;    // MAC to RLC
        input PHY_to_MAC;    // PHY to MAC
        output MAC_to_PHY;    // MAC to PHY
}

// 
// Base module for the MAC layer of LTE Stack.
//
simple LteMacBase like LteMac {
    parameters:
        @display("i=block/mac");
        string interfaceTableModule;

        //# Mac Queues
        int queueSize @unit(B) = default(2MiB);              // MAC Buffers queue size
        
        //# Mac MIB
        bool muMimo = default(true);
                       
        //# H-ARQ
        int harqProcesses = default(8);
        int maxHarqRtx = default(3);
        int harqFbEvaluationTimer = default(4);              // number of slots for sending back HARQ FB
        
        //# Statistics display (in GUI)
        bool statDisplay = default(false);
         
        //#
        //# Statistics recording
        //#
        @signal[macDelayDl];
        @statistic[macDelayDl](title="Delay at the MAC layer UL"; unit="s"; source="macDelayDl"; record=mean,vector);
        @signal[macThroughputDl];
        @statistic[macThroughputDl](title="Throughput at the MAC layer DL"; unit="Bps"; source="macThroughputDl"; record=mean);
        @signal[macDelayUl];
        @statistic[macDelayUl](title="Delay at the MAC layer UL"; unit="s"; source="macDelayUl"; record=mean,vector);
        @signal[macThroughputUl];
        @statistic[macThroughputUl](title="Throughput at the MAC layer UL"; unit="Bps"; source="macThroughputUl"; record=mean);
        @signal[macCellThroughputUl];
        @statistic[macCellThroughputUl](title="Cell Throughput at the MAC layer UL"; unit="Bps"; source="macCellThroughputUl"; record=mean);
        @signal[macCellThroughputDl];
        @statistic[macCellThroughputDl](title="Cell Throughput at the MAC layer DL"; unit="Bps"; source="macCellThroughputDl"; record=mean);
        @signal[macCellPacketLossDl];
        @statistic[macCellPacketLossDl](title="Mac Cell Packet Loss Dl"; unit=""; source="macCellPacketLossDl"; record=mean);
        @signal[macCellPacketLossUl];
        @statistic[macCellPacketLossUl](title="Mac Cell Packet Loss Ul"; unit=""; source="macCellPacketLossUl"; record=mean);
        @signal[macPacketLossUl];
        @statistic[macPacketLossUl](title="Mac Packet Loss Ul"; unit=""; source="macPacketLossUl"; record=mean);
        @signal[macPacketLossDl];
        @statistic[macPacketLossDl](title="Mac Packet Loss Dl"; unit=""; source="macPacketLossDl"; record=mean);
        @signal[macBufferOverFlowDl];
        @statistic[macBufferOverFlowDl](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowDl"; record=mean);
        @signal[macBufferOverFlowUl];
        @statistic[macBufferOverFlowUl](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowUl"; record=mean);
        @signal[macBufferOverFlowD2D];
    	@statistic[macBufferOverFlowD2D](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowD2D"; record=mean);
        @signal[harqErrorRateUl];
        @statistic[harqErrorRateUl](title="Harq Error Rate Ul"; unit=""; source="harqErrorRateUl"; record=mean,vector);
        @signal[harqErrorRateDl];
        @statistic[harqErrorRateDl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRateDl"; record=mean,vector);
        @signal[harqTxAttemptsDl];
        @statistic[harqTxAttemptsDl](title="Harq Tx Attempts Dl"; unit=""; source="harqTxAttemptsDl"; record=mean,vector);
        @signal[harqTxAttemptsUl];
        @statistic[harqTxAttemptsUl](title="Harq Tx Attempts Ul"; unit=""; source="harqTxAttemptsUl"; record=mean,vector);
        @signal[harqErrorRate_1st_Ul];
        @statistic[harqErrorRate_1st_Ul](title="Harq Error Rate Ul"; unit=""; source="harqErrorRate_1st_Ul"; record=mean,vector);
        @signal[harqErrorRate_1st_Dl];
        @statistic[harqErrorRate_1st_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_1st_Dl"; record=mean,vector);
        @signal[harqErrorRate_2nd_Ul];
        @statistic[harqErrorRate_2nd_Ul](title="Harq Error Rate Ul"; unit=""; source="harqErrorRate_2nd_Ul"; record=mean,vector);
        @signal[harqErrorRate_2nd_Dl];
        @statistic[harqErrorRate_2nd_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_2nd_Dl"; record=mean,vector);
        @signal[harqErrorRate_3rd_Ul];
        @statistic[harqErrorRate_3rd_Ul](title="Harq Error Rate Ul"; unit=""; source="harqErrorRate_3rd_Ul"; record=mean,vector);
        @signal[harqErrorRate_3rd_Dl];
        @statistic[harqErrorRate_3rd_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_3rd_Dl"; record=mean,vector);
        @signal[harqErrorRate_4th_Ul];
        @statistic[harqErrorRate_4th_Ul](title="Harq Error Rate Ul"; unit=""; source="harqErrorRate_4th_Ul"; record=mean,vector);
        @signal[harqErrorRate_4th_Dl];
        @statistic[harqErrorRate_4th_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_4th_Dl"; record=mean,vector);
        @signal[receivedPacketFromUpperLayer];
        @statistic[receivedPacketFromUpperLayer](source="receivedPacketFromUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[receivedPacketFromLowerLayer];
        @statistic[receivedPacketFromLowerLayer](source="receivedPacketFromLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToUpperLayer];
        @statistic[sentPacketToUpperLayer](source="sentPacketToUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToLowerLayer];
        @statistic[sentPacketToLowerLayer](source="sentPacketToLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[measuredItbs];
        @statistic[measuredItbs](title="TBS index"; unit=""; source="measuredItbs"; record=mean,vector);

    gates:
        //# 
        //# Gates connecting RLC and MAC Layers
        //# 
        
        input RLC_to_MAC;    // RLC to MAC
        output MAC_to_RLC;    // MAC to RLC
        
        //# 
        //# Gates connecting MAC and PHY Layers
        //# 
        
        input PHY_to_MAC;    // PHY to MAC
        output MAC_to_PHY;    // MAC to PHY
}

//
// User Equipment MAC layer of LTE stack
//
simple LteMacUe extends LteMacBase 
{
   
    parameters:
        @class("LteMacUe");
}

//
// D2D-capable User Equipment MAC layer of LTE stack
//
simple LteMacUeD2D extends LteMacUe {
    parameters:
        @class("LteMacUeD2D");
    
        bool usePreconfiguredTxParams = default(false);
        int d2dCqi = default(7);
        
        @signal[harqErrorRate_1st_D2D];
        @statistic[harqErrorRate_1st_D2D](title="Harq Error Rate D2D 1st Tx"; unit=""; source="harqErrorRate_1st_D2D"; record=mean,vector);
        @signal[harqErrorRate_2nd_D2D];
        @statistic[harqErrorRate_2nd_D2D](title="Harq Error Rate D2D 2nd Tx"; unit=""; source="harqErrorRate_2nd_D2D"; record=mean,vector);
        @signal[harqErrorRate_3rd_D2D];
        @statistic[harqErrorRate_3rd_D2D](title="Harq Error Rate D2D 3rd Tx"; unit=""; source="harqErrorRate_3rd_D2D"; record=mean,vector);
        @signal[harqErrorRate_4th_D2D];
        @statistic[harqErrorRate_4th_D2D](title="Harq Error Rate D2D 4th Tx"; unit=""; source="harqErrorRate_4th_D2D"; record=mean,vector);
        @signal[harqErrorRateD2D];
        @statistic[harqErrorRateD2D](title="Harq Error Rate D2D"; unit=""; source="harqErrorRateD2D"; record=mean,vector);
        @signal[macPacketLossD2D];
        @statistic[macPacketLossD2D](title="Mac Packet Loss D2D"; unit=""; source="macPacketLossD2D"; record=mean);
        @signal[macDelayD2D];
        @statistic[macDelayD2D](title="Delay at the MAC layer D2D"; unit="s"; source="macDelayD2D"; record=mean,vector);
        @signal[macThroughputD2D];
        @statistic[macThroughputD2D](title="Throughput at the MAC layer D2D"; unit="Bps"; source="macThroughputD2D"; record=mean);
        
        @signal[rcvdD2DModeSwitchNotification];
        @statistic[rcvdD2DModeSwitchNotification](title="Reception of mode switch notification (tx side)"; unit=""; source="rcvdD2DModeSwitchNotification"; record=count,vector);
}

//
// eNodeB MAC layer of LTE stack
//
simple LteMacEnb extends LteMacBase {
    
    parameters:
        
        @class("LteMacEnb");    
        
        //# volatile xml optSolution = default(xmldoc("solution.sol"));
        
        //#
        //# AMC Parameters
        //#
        
        // AMC Type:  "LteAmc", "NRAmc"
        string amcType = default("LteAmc");
        
        // AMC Mode:  "auto", "piloted", "multi", "das", "D2D"
        string amcMode = default("AUTO");
        
        //Min disance used to pair two MuMimo UE
        int muMimoMinDistance =default(50);
        
        // resource allocation type ("distributed" or "localized")
        string rbAllocationType = default("localized");

        // summary feedback confidence function lower bound
        double summaryLowerBound @unit(s) = default(5ms);

        // summary feedback confidence function upper bound
        double summaryUpperBound @unit(s) = default(20ms);

        // FeedBack Historical Base capacity in DL (number of stored feedback samples per UE) 
        int fbhbCapacityDl = default(5);

        // FeedBack Historical Base capacity in UL (number of stored feedback samples per UE)
        int fbhbCapacityUl = default(5);

        // FeedBack Historical Base capacity in D2D (number of stored feedback samples per UE)
        int fbhbCapacityD2D = default(5);
        
        // wideband PMI generation parameter (0.0 means "use the mean value" )        
        double pmiWeight = default(0.0);

        // wideband CQI generation parameter (0.0 means "use the mean value" )
        double cqiWeight = default(0.0);

        // AMC k CQI: used in DAS AMC mode
        int kCqi = default(0);
       
        // number of eNodeBs - set to 0 if unknown
        int eNodeBCount = default(0);
        //#
        //# eNb Scheduler Parameters
        //#    
        // Scheduling discipline. See LteCommon.h for discipline meaning.
        string schedulingDisciplineDl = default("MAXCI");
        string schedulingDisciplineUl = default("MAXCI");

        // Grant type DL
        string grantTypeConversationalDl = default("FITALL");
        string grantTypeStreamingDl      = default("FITALL");
        string grantTypeInteractiveDl    = default("FITALL");
        string grantTypeBackgroundDl     = default("FITALL");
        
        // Grant Size Dl in bytes (-1 means max grant, i.e. 4GB)
        int grantSizeConversationalDl = default(-1);
        int grantSizeStreamingDl      = default(-1);
        int grantSizeInteractiveDl    = default(-1);
        int grantSizeBackgroundDl     = default(-1);

        // Grant type Ul
        string grantTypeConversationalUl = default("FITALL");
        string grantTypeStreamingUl      = default("FITALL");
        string grantTypeInteractiveUl    = default("FITALL");
        string grantTypeBackgroundUl     = default("FITALL");

        // Grant Size Ul in bytes (-1 means max grant, i.e. 4GB)
        int grantSizeConversationalUl = default(-1);
        int grantSizeStreamingUl      = default(-1);
        int grantSizeInteractiveUl    = default(-1);
        int grantSizeBackgroundUl     = default(-1);
        
        // Proportional Fair parameters
        double pfAlpha    = default(0.95);
        
        // LTE Advanced Scheduler general parameters - DL
        int lteAallocationRbsDl = default(1);
        int lteAhistorySizeDL = default(20);
        int lteAgainHistoryThCONVERSATIONALDL = default(15);
        int lteAgainHistoryThSTREAMINGDL = default(15);
        int lteAgainHistoryThINTERACTIVEDL = default(15);
        int lteAgainHistoryThBACKGROUNDDL = default(15);
        
        // LTE Advanced Scheduler general parameters - UL 
        int lteAallocationRbsUl = default(1);
        int lteAhistorySizeUL = default(20);
        int lteAgainHistoryThUL = default(5);
            
        // LTE Advanced Scheduler QOS Parameters - DL
        double lteAdeadlineCONVERSATIONALDL @unit(s) = default(200ms);
        double lteAdeadlineSTREAMINGDL @unit(s) = default(1000ms);
        double lteAdeadlineINTERACTIVEDL @unit(s) = default(50ms);
        double lteAdeadlineBACKGROUNDDL @unit(s) = default(-1ms);
        
        double lteAslackTermCONVERSATIONALDL @unit(s) = default(50ms);
        double lteAslackTermSTREAMINGDL @unit(s) = default(100ms);
        double lteAslackTermINTERACTIVEDL @unit(s) = default(10ms);
        double lteAslackTermBACKGROUNDDL @unit(s) = default(0ms);
        
        int lteAmaxUrgentBurstCONVERSATIONALDL @unit(byte) = default(0B);
        int lteAmaxUrgentBurstSTREAMINGDL @unit(byte)     = default(0B);
        int lteAmaxUrgentBurstINTERACTIVEDL @unit(byte)    = default(0B);
        int lteAmaxUrgentBurstBACKGROUNDDL   @unit(byte)  = default(200B);
        
        int lteAmaxFairnessBurstCONVERSATIONALDL @unit(byte) = default(0B);
        int lteAmaxFairnessBurstSTREAMINGDL @unit(byte)     = default(0B);
        int lteAmaxFairnessBurstINTERACTIVEDL @unit(byte)    = default(0B);
        int lteAmaxFairnessBurstBACKGROUNDDL   @unit(byte)  = default(200B);
        
        // LTE Advanced Scheduler QOS Parameters - UL
        double lteAdeadlineCONVERSATIONALUL @unit(s) = default(250ms);
        double lteAdeadlineSTREAMINGUL @unit(s) = default(1500ms);
        double lteAdeadlineINTERACTIVEUL @unit(s) = default(2500ms);
        double lteAdeadlineBACKGROUNDUL @unit(s) = default(250ms);
        
        double lteAslackTermCONVERSATIONALUL @unit(s) = default(50ms);
        double lteAslackTermSTREAMINGUL @unit(s) = default(500ms);
        double lteAslackTermINTERACTIVEUL @unit(s) = default(500ms);
        double lteAslackTermBACKGROUNDUL @unit(s) = default(0ms);
        
        int lteAmaxBurstCONVERSATIONALUL @unit(byte) = default(0B);
        int lteAmaxBurstSTREAMINGUL     @unit(byte) = default(0B);
        int lteAmaxBurstINTERACTIVEUL   @unit(byte) = default(0B);
        int lteAmaxBurstBACKGROUNDUL    @unit(byte) = default(0B);
        
        // LTE Advanced Scheduler PF Version - DL
        bool pfTmsAwareDL = default(false);
        // LTE Advanced Scheduler PF Version - UL
        bool pfTmsAwareUL = default(false);
        

        // Power Model Parameters for MBSFN frame
        // minimum depleted power (W)
        double zeroLevelMBSFNDL @unit(W)= default(260W); //504
        // minimum depleted power (W)
        double zeroLevelMBSFNUL @unit(W)= default(260W); 
        // idle state depleted power (W)
        double idleLevelMBSFNDL @unit(W)= default(150W); //336
        // idle state depleted power (W)
        double idleLevelMBSFNUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitMBSFNDL @unit(W)= default(3.76W); //6.72
        // per-block depletion unit (W)
        double powerUnitMBSFNUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerMBSFNDL @unit(W)= default(448W); //840
        // maximumum depletable power (W)
        double maxPowerMBSFNUL @unit(W)= default(448W);
        
        // Power Model Parameters for PAGING FRAMES
        // minimum depleted power (W)
        double zeroLevelPAGINGDL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelPAGINGUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelPAGINGDL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelPAGINGUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitPAGINGDL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitPAGINGUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerPAGINGDL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerPAGINGUL @unit(W)= default(448W);
        
        // Power Model Parameters for NORMAL frames
        // minimum depleted power (W)
        double zeroLevelNORMAL_FRAME_TYPEDL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelNORMAL_FRAME_TYPEUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelNORMAL_FRAME_TYPEDL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelNORMAL_FRAME_TYPEUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitNORMAL_FRAME_TYPEDL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitNORMAL_FRAME_TYPEUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerNORMAL_FRAME_TYPEDL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerNORMAL_FRAME_TYPEUL @unit(W)= default(448W);
        
        // Power Model Parameters for BROADCAST frames
        // minimum depleted power (W)
        double zeroLevelBROADCASTDL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelBROADCASTUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelBROADCASTDL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelBROADCASTUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitBROADCASTDL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitBROADCASTUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerBROADCASTDL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerBROADCASTUL @unit(W)= default(448W);
        
        // Power Model Parameters for SYNCRO frames
        // minimum depleted power (W)
        double zeroLevelSYNCRODL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelSYNCROUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelSYNCRODL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelSYNCROUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitSYNCRODL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitSYNCROUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerSYNCRODL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerSYNCROUL @unit(W)= default(448W);
        
        double prfFixedSYNCRO @unit(W)= default(5.543W);
        double prfRBSYNCRO @unit(W)= default(0.689W);
        
        double prfFixedMBSFN @unit(W)=default(4.4444W);
        double prfRBMBSFN @unit(W)=default(0.711W);
        
        double prfFixedNORMAL_FRAME_TYPE @unit(W)= default(3.810W);
        double prfRBNORMAL_FRAME_TYPE @unit(W)= default(0.724W);
        
        double prfFixedPAGING @unit(W)= default(3.810W);
        double prfRBPAGING @unit(W)= default(0.724W);
        
        double prfFixedBROADCAST @unit(W)= default(4.4W);
        double prfRBBROADCAST @unit(W)= default(0.712W);
        
        string pilotMode = default("ROBUST_CQI"); // one of MIN_CQI, MAX_CQI, AVG_CQI, ROBUST_CQI
        
        //#
        //# Statistics for Lte Scheduler Enb
        @signal[cellBlocksUtilizationDl];
        @statistic[cellBlocksUtilizationDl](title="LTE Cell Blocks Utilization Dl"; unit="blocks"; source="cellBlocksUtilizationDl"; record=mean);
        @signal[cellBlocksUtilizationUl];
        @statistic[cellBlocksUtilizationUl](title="LTE Cell Blocks Utilization Ul"; unit="blocks"; source="cellBlocksUtilizationUl"; record=mean);
        @signal[avgServedBlocksDl];
        @statistic[avgServedBlocksDl](title="LTE Avg Served Blocks Dl"; unit="blocks"; source="avgServedBlocksDl"; record=mean,vector);
        @signal[avgServedBlocksUl];
        @statistic[avgServedBlocksUl](title="LTE Avg Served Blocks Ul"; unit="blocks"; source="avgServedBlocksUl"; record=mean,vector);
}    

//
// eNodeB MAC layer of LTE stack with support for D2D-capable UEs
//
simple LteMacEnbD2D extends LteMacEnb 
{
    parameters:
        @class("LteMacEnbD2D"); 
       
	    bool usePreconfiguredTxParams = default(false);
        int d2dCqi = default(7);
        
        // frequency reuse parameters
        bool reuseD2D = default(false);
        bool reuseD2DMulti = default(false);
        double conflictGraphUpdatePeriod @unit(s) = default(1s);
        
        // CG thresholds can be defined in either dBm or meters
        // if distances are set to -1.0, then dBm thresholds are used,
        // otherwise distances have priority on dBm thresholds 
        double conflictGraphThreshold = default(90);                       // dBm
        double conflictGraphD2DInterferenceRadius @unit(m) = default(-1.0m);         // meters
        double conflictGraphD2DMultiTxRadius @unit(m) = default(-1.0m);              // meters
        double conflictGraphD2DMultiInterferenceRadius @unit(m) = default(-1.0m);    // meters
        
        // handling of D2D mode switch
        bool msHarqInterrupt = default(true);
        bool msClearRlcBuffer = default(true);
        
        @signal[macCellThroughputD2D];
        @statistic[macCellThroughputD2D](title="Cell Throughput at the MAC layer D2D"; unit="Bps"; source="macCellThroughputD2D"; record=mean); 
        @signal[macCellPacketLossD2D];
        @statistic[macCellPacketLossD2D](title="Mac Cell Packet Loss D2D"; unit=""; source="macCellPacketLossD2D"; record=mean);
}
