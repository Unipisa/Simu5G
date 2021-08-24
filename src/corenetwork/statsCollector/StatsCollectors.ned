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

package simu5g.corenetwork.statsCollector;
// 
// This is the statistic collector of an eNodeB. It stores all the attributes of the RNI 
// service Layer2Meaurements resource. The RNI service will call its methods in order to 
// responds to requests

//

moduleinterface StatsCollector {
    string collectorType; // it can be: "eNBStatsCollector, gNBStatsCollector, ueStatsCollector, NRueStatsCollector
}

module BaseStationStatsCollector like StatsCollector
{
    parameters:
        string collectorType = default("eNBStatsCollector");

		bool movingAverage = default(false);
        
        // used to calculate l2meas as averages 
        int prbUsagePeriods = default(15);
        int dataVolumePeriods = default(15);
        int activeUserPeriods = default(15); //10*100ms = 1s
        int discardRatePeriods = default(15);
        int tPutPeriods = default(15);

		// period duration for each l2 meas
		double prbUsagePeriod  @unit(s)   = default(0.01s); // TTI period
        double activeUserPeriod @unit(s)  = default(0.1s);
        double dataVolumePeriod @unit(s)   = default(0.5s);
        double delayPacketPeriod @unit(s)  = default(0.05s);
        double discardRatePeriod @unit(s) = default(10s);
        double tPutPeriod @unit(s) = default(1s);

        @display("i=block/cogwheel");
        @class("BaseStationStatsCollector");
}

module GNodeBStatsCollector extends BaseStationStatsCollector
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
        int dataVolumePeriods = default(15);
        int delayPacketPeriods = default(10);
        int discardRatePeriods = default(15);
        int tPutPeriods = default(15);
        
        int numberOfSamples = default(20);
        
        @display("i=block/cogwheel");
        @class("UeStatsCollector");   
}

module NRueStatsCollector extends UeStatsCollector
{
	parameters:
	    collectorType = default("NRueStatsCollector");
}










