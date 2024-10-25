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

package simu5g.nodes.backgroundCell;

//
// This module is responsible for computing the allocation of time-frequency resources
// for background UEs in a ~BackgroundCell, taking into account the traffic generated
// by these UEs and their channel quality.
//
simple BackgroundScheduler
{
    parameters:
        @display("i=block/control");
        @class("BackgroundScheduler");

        string binderModule = default("binder");
        string trafficManagerModule = default("^.bgTrafficGenerator.manager");
        string channelModelModule = default("^.bgChannelModel");

        // transmission power
        double txPower @unit(dBm) = default(46dBm);

        // transmission direction
        string txDirection @enum(ANISOTROPIC,OMNI) = default("OMNI");

        // transmission angle
        double txAngle @unit(deg) = default(0deg);

        // flag to indicate whether this is a NR base station
        bool isNr = default(false);

        // operating carrier
        int numerologyIndex = default(0);

        // operating carrier
        double carrierFrequency @unit(GHz) = default(2GHz);

        // carrier bandwidth
        int numBands = default(6);

        // statistics
        @signal[bgAvgServedBlocksDl];
        @statistic[bgAvgServedBlocksDl](title="Average DL served blocks for background cells"; unit=""; source="bgAvgServedBlocksDl"; record=mean,vector);
        @signal[bgAvgServedBlocksUl];
        @statistic[bgAvgServedBlocksUl](title="Average UL served blocks for background cells"; unit=""; source="bgAvgServedBlocksUl"; record=mean,vector);
}

