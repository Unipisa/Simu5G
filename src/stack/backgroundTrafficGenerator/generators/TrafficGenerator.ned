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

package simu5g.stack.backgroundTrafficGenerator.generators;

import inet.mobility.contract.IMobility;

moduleinterface ITrafficGenerator
{
    parameters:
        @display("i=block/source");
}

simple TrafficGeneratorBase like ITrafficGenerator 
{
    parameters:
        @display("i=block/source");
        @class("TrafficGeneratorBase");
        
        double startTimeDl @unit("s") = default(-1s);
        double startTimeUl @unit("s") = default(-1s);
        
        //# TODO check parameters
        int headerLen @unit(B) = default(33B); 
        double txPower @unit(dBm) = default(26dBm); 
        int fbPeriod = default(6); 
        
        volatile double periodDl @unit("s") = default(uniform(10ms,50ms));
        volatile double periodUl @unit("s") = default(uniform(10ms,50ms));
        
        volatile int packetSizeDl @unit(B) = default(intuniform(20B,100B));
        volatile int packetSizeUl @unit(B) = default(intuniform(20B,100B));
        
        double rtxRateDl = default(0.1);
        double rtxDelayDl @unit("s")= default(0.006s);
        double rtxRateUl = default(0.1);
        double rtxDelayUl @unit("s")= default(0.006s);
              
        // these values are only considered when
        // - periodicCqi == false && computeAvgInterference == false 
        double cqiMeanDl = default(15);
        double cqiMeanUl = default(15);
        double cqiStddevDl = default(0);
        double cqiStddevUl = default(0);  
        
        // statistics 
        @signal[bgMeasuredSinrDl];
        @statistic[bgMeasuredSinrDl](title="Average DL SINR for background UEs"; unit=""; source="bgMeasuredSinrDl"; record=mean,count,vector);
        @signal[bgMeasuredSinrUl];
        @statistic[bgMeasuredSinrUl](title="Average UL SINR for background UEs"; unit=""; source="bgMeasuredSinrUl"; record=mean,count,vector);  
        @signal[bgAverageCqiDl];
        @statistic[bgAverageCqiDl](title="Average DL CQI for background UEs"; unit=""; source="bgAverageCqiDl"; record=mean,count,vector);
        @signal[bgAverageCqiUl];
        @statistic[bgAverageCqiUl](title="Average UL CQI for background UEs"; unit=""; source="bgAverageCqiUl"; record=mean,count,vector);
        @signal[bgHarqErrorRateDl];
        @statistic[bgHarqErrorRateDl](title="Average DL HARQ error rate for background UEs"; unit=""; source="bgHarqErrorRateDl"; record=mean,count,vector);
        @signal[bgHarqErrorRateUl];
        @statistic[bgHarqErrorRateUl](title="Average UL HARQ error rate for background UEs"; unit=""; source="bgHarqErrorRateUl"; record=mean,count,vector);
}

//# add here your custom generators, by extending the base module

simple TrafficGeneratorCbr extends TrafficGeneratorBase
{
    parameters:
        @display("i=block/source");
        @class("TrafficGeneratorCbr");
        
        
}