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
simple LteDlFeedbackGenerator {
    parameters:
        @display("i=block/cogwheel");
        
        // can be ALLBANDS, PREFERRED, WIDEBAND
        string feedbackType = default("ALLBANDS");
                
        // resource allocation type ("distributed" or "localized")
        string rbAllocationType = default("localized");    
        
        // period for Periodic feedback in TTI
        int fbPeriod = default(6);         
        
        // time interval between sensing and transmission in TTI
        int fbDelay  = default(1);         
        
        // true if we want to use also periodic feedback
        bool usePeriodic = default(true);  
        
        // initial txMode (see LteCommon.h)
        //     SINGLE_ANTENNA_PORT0,SINGLE_ANTENNA_PORT5,TRANSMIT_DIVERSITY,OL_SPATIAL_MULTIPLEXING,
        // CL_SPATIAL_MULTIPLEXING,MULTI_USER,
        string initialTxMode = default("SINGLE_ANTENNA_PORT0");  
        
        //Feedback Computation (warning, this should be compatible with
        // analogue model and Decider
//        xml feedbackComputation;
        
        //Type of generator: ideal, real, das_aware
        //ideal: feedback generator reports feedback for each txmode and for each rus
        //real: feedback generator reports feedback only for the last txmode used but for each rus
        //das_aware: feedback generator reports feedback only for the last txmode used and only for rus in Antenna set
        string feedbackGeneratorType= default("IDEAL");
}

// 
// This is the Lte Uplink Feedback Generator.
//
simple LteUlFeedbackGenerator {
    parameters:
        @display("i=block/cogwheel");
        // TODO
}   
    