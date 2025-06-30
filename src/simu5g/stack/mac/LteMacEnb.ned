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
// This module extends the ~LteMacBase module by providing specific MAC functions
// on the eNodeB side. Specifically, it initializes and manages the scheduling classes
// responsible for resource allocation operations in both uplink and downlink directions,
// while also supporting Carrier Aggregation (CA).
// It manages the Random Access procedure (RAC) and the reception of Buffer Status
// Reports (BSRs) from UEs, as well as the transmission of scheduling grants to UEs allocated
// within the uplink bandwidth. For downlink connections, it notifies the Radio Link Control (RLC)
// layer when it is ready to receive MAC Service Data Units (SDUs) to build MAC
// Protocol Data Units (PDUs) and transmit them to the physical layer.
//
simple LteMacEnb extends LteMacBase
{
    parameters:
        @class("LteMacEnb");

        //# volatile xml optSolution = default(xmldoc("solution.sol"));

        //#
        //# AMC Parameters
        //#

        // AMC Type:  "LteAmc", "NRAmc"
        string amcType @enum(LteAmc,NRAmc) = default("LteAmc");

        // AMC Mode:  "auto", "piloted", "multi", "das", "D2D"
        string amcMode @enum(AUTO,D2D) = default("AUTO");

        // resource allocation type ("distributed" or "localized")
        string rbAllocationType @enum(distributed,localized) = default("localized");

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

        // number of eNodeBs - set to 0 if unknown
        int eNodeBCount = default(0);

        //#
        //# eNb Scheduler Parameters
        //#
        // Scheduling discipline. See LteCommon.h for discipline meaning.
        string schedulingDisciplineDl @enum(DRR,PF,MAXCI,MAXCI_MB,MAXCI_OPT_MB,MAXCI_COMP,ALLOCATOR_BESTFIT) = default("MAXCI");
        string schedulingDisciplineUl @enum(DRR,PF,MAXCI,MAXCI_MB,MAXCI_OPT_MB,MAXCI_COMP,ALLOCATOR_BESTFIT) = default("MAXCI");

        // Proportional Fair parameters
        double pfAlpha = default(0.95);

        string pilotMode @enum(IN_CQI,MAX_CQI,AVG_CQI,MEDIAN_CQI,ROBUST_CQI) = default("ROBUST_CQI");

        string cellInfoModule;
        string rlcUmModule = default("^.rlc.um");
        string pdcpRrcModule = default("^.pdcpRrc");

        //#
        //# Statistics related to resource blocks occupancy
        @signal[avgServedBlocksDl];
        @statistic[avgServedBlocksDl](title="Average number of allocated Resource Blocks in the Dl"; unit="blocks"; source="avgServedBlocksDl"; record=mean,vector);
        @signal[avgServedBlocksUl];
        @statistic[avgServedBlocksUl](title="Average number of allocated Resource Blocks in the Dl"; unit="blocks"; source="avgServedBlocksUl"; record=mean,vector);
}

