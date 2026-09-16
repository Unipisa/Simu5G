//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
// Editor: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


package simu5g.stack.mac;

import simu5g.stack.mac.amc.ILteAmc;
import simu5g.stack.mac.scheduler.ILteSchedulerEnbDl;
import simu5g.stack.mac.scheduler.ILteSchedulerEnbUl;
import simu5g.stack.mac.scheduler.LteSchedulerEnbDl;

//
// Extends the ~LteMacBase module by providing specific MAC functions
// on the eNodeB side. Specifically, it initializes and manages the scheduling classes
// responsible for resource allocation operations in both uplink and downlink directions,
// while also supporting Carrier Aggregation (CA).
// It manages the Random Access procedure (RAC) and the reception of Buffer Status
// Reports (BSRs) from UEs, as well as the transmission of scheduling grants to UEs allocated
// within the uplink bandwidth. For downlink connections, it notifies the Radio Link Control (RLC)
// layer when it is ready to receive MAC Service Data Units (SDUs) to build MAC
// Protocol Data Units (PDUs) and transmit them to the physical layer.
//
module LteMacEnb extends LteMacBase
{
    parameters:
        @class("LteMacEnb");

        // Module references for accessing other stack layers
        string rlcMuxModule = default("^.rlcMux");
        string bearerManagementModule = default("^.rrc.bearerManagement");

        //# volatile xml optSolution = default(xmldoc("solution.sol"));

        //# AMC Parameters

        // AMC Type:  "LteAmc", "NrAmc" (the D2D NICs swap in the "LteAmcD2D"/"NrAmcD2D" leaves)
        string amcType @enum(LteAmc,NrAmc,LteAmcD2D,NrAmcD2D) = default("LteAmc");

        // AMC pilot mode; supported values depend on the AMC implementation (LteAmc: "AUTO", "D2D")
        string amcMode = default("AUTO");

        // Resource allocation type ("distributed" or "localized")
        string rbAllocationType @enum(distributed,localized) = default("localized");

        // Summary feedback confidence function lower bound
        double summaryLowerBound @unit(s) = default(5ms);

        // Summary feedback confidence function upper bound
        double summaryUpperBound @unit(s) = default(20ms);

        // Feedback historical base capacity in DL (number of stored feedback samples per UE)
        int fbhbCapacityDl = default(5);

        // Feedback historical base capacity in UL (number of stored feedback samples per UE)
        int fbhbCapacityUl = default(5);

        // Wideband CQI generation parameter (0.0 means "use the mean value")
        double cqiWeight = default(0.0);

        // Number of eNodeBs - set to 0 if unknown
        int eNodeBCount = default(0);

        //# eNB scheduler parameters
        // Scheduling discipline. See LteCommon.h for discipline meaning.
        // ALLOCATOR_BESTFIT is the D2D frequency-reuse allocator: it is only
        // constructible by the D2D uplink schedulers, so selecting it requires
        // ulSchedulerType = LteSchedulerEnbUlD2D or NrSchedulerGnbUlD2D (and hence
        // the Simu5G_D2D feature). Any other scheduler raises an error at setup.
        string schedulingDisciplineDl @enum(DRR,PF,MAXCI,MAXCI_MB,MAXCI_OPT_MB,MAXCI_COMP,ALLOCATOR_BESTFIT,QOS_PF) = default("MAXCI");
        string schedulingDisciplineUl @enum(DRR,PF,MAXCI,MAXCI_MB,MAXCI_OPT_MB,MAXCI_COMP,ALLOCATOR_BESTFIT,QOS_PF) = default("MAXCI");

        // Scheduler Types
        string ulSchedulerType @enum(LteSchedulerEnbUl,NrSchedulerGnbUl,LteSchedulerEnbUlD2D,NrSchedulerGnbUlD2D) = default("LteSchedulerEnbUl");
        string dlSchedulerType @enum(LteSchedulerEnbDl) = default("LteSchedulerEnbDl");

        // Proportional Fair parameters
        double pfAlpha = default(0.95);

        string pilotMode @enum(IN_CQI,MAX_CQI,AVG_CQI,MEDIAN_CQI,ROBUST_CQI) = default("ROBUST_CQI");

        int numPreambles = default(64);        // Number of RACH preambles available for contention-based random access

        string cellInfoModule;

        //# Statistics related to resource blocks occupancy
        @signal[avgServedBlocksDl];
        @statistic[avgServedBlocksDl](title="Average number of allocated Resource Blocks in the Dl"; unit="blocks"; source="avgServedBlocksDl"; record=mean,vector);
        @signal[avgServedBlocksUl];
        @statistic[avgServedBlocksUl](title="Average number of allocated Resource Blocks in the Dl"; unit="blocks"; source="avgServedBlocksUl"; record=mean,vector);

    submodules:
        amc: <amcType> like ILteAmc {
            @display("p=60,140");
        }
        schedulerDl: <dlSchedulerType> like ILteSchedulerEnbDl {
            @display("p=60,60");
        }
        schedulerUl: <ulSchedulerType> like ILteSchedulerEnbUl {
            @display("p=180,60");
        }
}

