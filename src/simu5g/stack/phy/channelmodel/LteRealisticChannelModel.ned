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

package simu5g.stack.phy.channelmodel;

//
// This module implements the functions for a model representing the 2D channel in the LTE
// specification 3GPP TR 36.803. It includes the computation of:
// - path loss (both line-of-sight and non-line-of-sight),
// - fading (Jakes or Rayleigh model),
// - log-normal shadowing,
// - inter-cell interference.
// These effects are utilized to compute the Signal to Interference-plus-Noise Ratio (SINR).
//
//
simple LteRealisticChannelModel extends LteChannelModel
{
    parameters:
        @class("LteRealisticChannelModel");

        // Enable/disable shadowing
        bool shadowing = default(true);

        // Pathloss scenario from ITU -->
        string scenario @enum(INDOOR_HOTSPOT,URBAN_MICROCELL,URBAN_MACROCELL,RURAL_MACROCELL,SUBURBAN_MACROCELL,UNKNOW_SCENARIO) = default("URBAN_MACROCELL");

        // eNodeB height -->
        double nodebHeight @unit(m) = default(25m);
        // Building height -->
        double buildingHeight @unit(m) = default(20m);
        // determines if the UE is inside a building
        bool insideBuilding = default(false);

        double streetWidth @unit(m) = default(20m);
        double ueHeight @unit(m) = default(1.5m);
        bool tolerateMaxDistViolation = default(false);
        bool useTorus = default(false);

        double correlationDistance @unit(m) = default(50m);

        // Target bler used to compute feedback -->
        double targetBler = default(0.01);
        // HARQ reduction -->
        double harqReduction = default(0.2);

        // Rank indicator tracefile -->
        double lambdaMinTh = default(0.02);
        double lambdaMaxTh = default(0.2);
        double lambdaRatioTh = default(20);

        // Antenna Gain of UE -->
        double antennaGainUe @unit(dBi) = default(0dBi);
        // Antenna Gain of eNodeB -->
        double antennGainEnB @unit(dBi) = default(18dBi);
        // Antenna Gain of Micro node -->
        double antennGainMicro @unit(dBi) = default(5dBi);
        // Thermal Noise for 10 MHz of Bandwidth -->
        double thermalNoise @unit(dBm) = default(-104.5dBm);
        // Ue noise figure -->
        double ueNoiseFigure @unit(dBm) = default(7dBm);
        // eNodeB noise figure -->
        double bsNoiseFigure @unit(dBm) = default(5dBm);
        // Cable Loss -->
        double cableLoss @unit(dB) = default(2dB);

        // If true enable the possibility to switch dinamically the LOS/NLOS pathloss computation -->
        bool dynamicLos = default(false);
        // If dynamic-los is false this parameter, if true, compute LOS pathloss otherwise compute NLOS pathloss -->
        bool fixedLos = default(false);
        // Enable/disable fading -->
        bool fading = default(true);
        // Fading type (JAKES or RAYGHLEY) -->
        string fadingType @enum(RAYLEIGH,JAKES) = default("JAKES");
        // If jakes fading this parameter specify the number of path (tap channel) -->
        int numFadingPaths = default(6);

        double delayRms @unit(s) = default(363ns);

        // if true, enables the inter-cell interference computation for UL and DL connections from background cells -->
        bool bgCellInterference = default(true);
        // if true, enables the inter-cell interference computation for DL connections from external cells (maybe this is obsolete and should be removed) -->
        bool extCellInterference = default(true);
        // if true, enables the inter-cell interference computation for DL connections -->
        bool downlinkInterference = default(false);
        // if true, enables the interference computation for UL connections -->
        bool uplinkInterference = default(false);
	// if true, enables the interference computation for D2D connections -->
        bool d2dInterference = default(true);

        bool enableExtCellLos = default(true);

        // collection of SINR statistics can be disabled because it might be quite time-consuming
        bool collectSinrStatistics = default(true);

        // statistics
        @signal[rcvdSinrDl];
        @statistic[rcvdSinrDl](title="SINR measured at packet reception, DL"; unit="dB"; source="rcvdSinrDl"; record=mean,vector);
        @signal[rcvdSinrUl];
        @statistic[rcvdSinrUl](title="SINR measured at packet reception, UL"; unit="dB"; source="rcvdSinrUl"; record=mean,vector);
        @signal[rcvdSinrD2D];
        @statistic[rcvdSinrD2D](title="SINR measured at packet reception, D2D"; unit="dB"; source="rcvdSinrD2D"; record=mean,vector);

        @signal[measuredSinrDl];
        @statistic[measuredSinrDl](title="SINR measured at feedback computation, DL"; unit="dB"; source="measuredSinrDl"; record=mean,vector);
        @signal[measuredSinrUl];
        @statistic[measuredSinrUl](title="SINR measured at feedback computation, UL"; unit="dB"; source="measuredSinrUl"; record=mean,vector);
}

