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
// Medium Access Control (MAC) layer for the evolved Node B (eNodeB) in an LTE network.
// This module is responsible for managing all data transmission aspects at the 
// MAC sublayer within the eNodeB, including scheduling, resource allocation, 
// and handling of data flow between the physical layer and upper layers. It aims 
// to provide efficient, fair, and reliable communication for connected user equipments (UEs).
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

        //Min disance used to pair two MuMimo UE
        int muMimoMinDistance = default(50);

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

