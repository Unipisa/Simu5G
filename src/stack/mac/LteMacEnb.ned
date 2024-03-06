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
// eNodeB MAC layer of LTE stack
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
        string amcType = default("LteAmc");

        // AMC Mode:  "auto", "piloted", "multi", "das", "D2D"
        string amcMode = default("AUTO");

        //Min disance used to pair two MuMimo UE
        int muMimoMinDistance = default(50);

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

        // number of eNodeBs - set to 0 if unknown
        int eNodeBCount = default(0);

        //#
        //# eNb Scheduler Parameters
        //#    
        // Scheduling discipline. See LteCommon.h for discipline meaning.
        string schedulingDisciplineDl = default("MAXCI");
        string schedulingDisciplineUl = default("MAXCI");

        // Proportional Fair parameters
        double pfAlpha = default(0.95);

        string pilotMode = default("ROBUST_CQI"); // one of MIN_CQI, MAX_CQI, AVG_CQI, ROBUST_CQI

        //#
        //# Statistics related to resource blocks occupancy
        @signal[avgServedBlocksDl];
        @statistic[avgServedBlocksDl](title="Average number of allocated Resource Blocks in the Dl"; unit="blocks"; source="avgServedBlocksDl"; record=mean,vector);
        @signal[avgServedBlocksUl];
        @statistic[avgServedBlocksUl](title="Average number of allocated Resource Blocks in the Dl"; unit="blocks"; source="avgServedBlocksUl"; record=mean,vector);
}

