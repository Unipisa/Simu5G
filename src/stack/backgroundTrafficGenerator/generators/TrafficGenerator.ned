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
        
        // statistics        
        @signal[bgAverageCqiDl];
        @statistic[bgAverageCqiDl](title="Average DL CQI for background UEs"; unit=""; source="bgAverageCqiDl"; record=mean);
        @signal[bgAverageCqiUl];
        @statistic[bgAverageCqiUl](title="Average UL CQI for background UEs"; unit=""; source="bgAverageCqiUl"; record=mean);
}

//# add here your custom generators, by extending the base module

simple TrafficGeneratorCbr extends TrafficGeneratorBase
{
    parameters:
        @display("i=block/source");
        @class("TrafficGeneratorCbr");
        
        
}