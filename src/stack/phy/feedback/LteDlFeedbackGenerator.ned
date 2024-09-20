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


package simu5g.stack.phy.feedback;

//
// This is the Lte Downlink Feedback Generator.
// It manages periodic and aperiodic channel sensing and
// feedback transmission.
//
simple LteDlFeedbackGenerator
{
    parameters:
        @display("i=block/cogwheel");

        // can be ALLBANDS, PREFERRED, WIDEBAND
        string feedbackType @enum(ALLBANDS,PREFERRED,WIDEBAND) = default("ALLBANDS");

        // resource allocation type ("distributed" or "localized")
        string rbAllocationType @enum(distributed,localized) = default("localized");

        // period for Periodic feedback in TTI
        int fbPeriod @unit(tti) = default(6tti); 

        // time interval between sensing and transmission in TTI
        int fbDelay @unit(tti) = default(1tti);

        // true if we want to use also periodic feedback
        bool usePeriodic = default(true);

        // initial txMode (see LteCommon.h)
        //     SINGLE_ANTENNA_PORT0,SINGLE_ANTENNA_PORT5,TRANSMIT_DIVERSITY,OL_SPATIAL_MULTIPLEXING,
        // CL_SPATIAL_MULTIPLEXING,MULTI_USER,
        string initialTxMode @enum(SINGLE_ANTENNA_PORT0,SINGLE_ANTENNA_PORT5,TRANSMIT_DIVERSITY,OL_SPATIAL_MULTIPLEXING,CL_SPATIAL_MULTIPLEXING,MULTI_USER) = default("SINGLE_ANTENNA_PORT0");

        // Feedback Computation (warning: this should be compatible with
        // the analogue model and Decider.
        // xml feedbackComputation;

        //Type of generator: IDEAL, REAL, DAS_AWARE
        // IDEAL: feedback generator reports feedback for each txmode and for each rus
        // REAL: feedback generator reports feedback only for the last txmode used but for each rus
        // DAS_AWARE: feedback generator reports feedback only for the last txmode used and only for rus in Antenna set
        string feedbackGeneratorType @enum(IDEAL,REAL,DAS_AWARE) = default("IDEAL");

        string binderModule = default("binder");
        string phyModule = default("^.phy");
}

