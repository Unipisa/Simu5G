
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
// This module collects and maintains statistics for User Equipment (UE) within an eNodeB or gNodeB.
// It gathers Layer 2 measurements for the Radio Network Information (RNI) Service. This includes
// metrics such as packet delays, discard rates, throughput, and data volume. The collected 
// data is utilized to respond to RNI service requests effectively and is managed through 
// defined time periods for various measurements.
//
module UeStatsCollector like IStatsCollector
{
    parameters:
        string collectorType = default("ueStatsCollector");
        string binderModule = default("binder");
        string interfaceTableModule;
        string macModule = default("^.cellularNic.mac");
        // string pdcpRrcModule = default("^.cellularNic.pdcpRrc");
        string packetFlowManagerModule = default("^.cellularNic.packetFlowManager");
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

