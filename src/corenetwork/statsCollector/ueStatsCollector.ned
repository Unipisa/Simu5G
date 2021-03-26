// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.statsCollector;
//
// The UeStatsCollector retrieves some L2 measures used for the RNI service of
// the MEC framework. In particular it retrieves packet delays and discard rate.
// It is managed by the eNodeBStatsCollector modules. The latter has timers that
// periodically calculate the measures.
//

simple UeStatsCollector 
{
    parameters:
        string interfaceTableModule;
        bool movingAverage = default(false);
        
        // time periods for measurements 
        int dataVolumePeriods = default(2);
        int delayPacketPeriods = default(2); //used by the ue collector, remove 
        int discardRatePeriods = default(2);
        int tPutPeriods = default(2);
        
        int numberOfSamples = default(20);
        
         
        
        @display("i=block/cogwheel");
}