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
// This is the statistic collector of an eNodeB. It stores all the attributes of the RNI 
// service Layer2Meaurements resource. The RNI service will call its methods in order to 
// responds to requests

//

moduleinterface StatsCollector {
    string collectorType; // it can be: "eNBStatsCollector, gNBStatsCollector, ueStatsCollector, NRueStatsCollector
}

module ENodeBStatsCollector like StatsCollector
{
    parameters:
        string collectorType = default("eNBStatsCollector");

		bool movingAverage = default(false);
        
        // used to calculate l2meas as averages 
        int prbUsagePeriods = default(5);
        int dataVolumePeriods = default(2);
        int activeUserPeriods = default(10); //10*100ms = 1s
        int discardRatePeriods = default(2);
        int tPutPeriods = default(2);

		// period duration for each l2 meas
		double prbUsagePeriod  @unit(s)   = default(0.5s); // TTI period
        double activeUserPeriod @unit(s)  = default(0.1s);
        double dataVolumePeriod @unit(s)   = default(0.5s);
        double delayPacketPeriod @unit(s)  = default(0.05s);
        double discardRatePeriod @unit(s) = default(60s);
        double tPutPeriod @unit(s) = default(1s);

        @display("i=block/cogwheel");
        @class("EnodeBStatsCollector");
}

module GNodeBStatsCollector extends ENodeBStatsCollector
{
    parameters:
        collectorType = default("gNBStatsCollector");
}

module UeStatsCollector like StatsCollector
{
    parameters:
        string collectorType = default("ueStatsCollector");
        string interfaceTableModule;
        bool movingAverage = default(true);
        
        // time periods for measurements 
        int dataVolumePeriods = default(2);
        int delayPacketPeriods = default(10);
        int discardRatePeriods = default(2);
        int tPutPeriods = default(2);
        
        int numberOfSamples = default(20);
        
        @display("i=block/cogwheel");
        @class("UeStatsCollector");   
}

module NRueStatsCollector extends UeStatsCollector
{
	parameters:
	    collectorType = default("NRueStatsCollector");
}










