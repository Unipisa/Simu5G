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
// This module collects the radio network information of a LTE User Equipment (UE), which can
// be accessed by the ~RNIService module of a ~MECHost. For example, this module
// provides information such as packet delay and discard rate.
//
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

