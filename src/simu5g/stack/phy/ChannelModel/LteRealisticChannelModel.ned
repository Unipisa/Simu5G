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

package simu5g.stack.phy.ChannelModel;

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
        double nodeb_height @unit(m) = default(25m);
        // Building height -->
        double building_height @unit(m) = default(20m);
        // determines if the UE is inside a building
        bool inside_building = default(false);

        double street_wide @unit(m) = default(20m);
        double ue_height @unit(m) = default(1.5m);
        bool tolerateMaxDistViolation = default(false);
        bool useTorus = default(false);

        double correlation_distance @unit(m) = default(50m);

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
        double ue_noise_figure @unit(dBm) = default(7dBm);
        // eNodeB noise figure -->
        double bs_noise_figure @unit(dBm) = default(5dBm);
        // Cable Loss -->
        double cable_loss @unit(dB) = default(2dB);

        // If true enable the possibility to switch dinamically the LOS/NLOS pathloss computation -->
        bool dynamic_los = default(false);
        // If dynamic-los is false this parameter, if true, compute LOS pathloss otherwise compute NLOS pathloss -->
        bool fixed_los = default(false);
        // Enable/disable fading -->
        bool fading = default(true);
        // Fading type (JAKES or RAYGHLEY) -->
        string fading_type @enum(RAYLEIGH,JAKES) = default("JAKES");
        // If jakes fading this parameter specify the number of path (tap channel) -->
        int fading_paths = default(6);

        double delay_rms @unit(s) = default(363ns);

        // if true, enables the inter-cell interference computation for UL and DL connections from background cells -->
        bool bgCell_interference = default(true);
        // if true, enables the inter-cell interference computation for DL connections from external cells (maybe this is obsolete and should be removed) -->
        bool extCell_interference = default(true);
        // if true, enables the inter-cell interference computation for DL connections -->
        bool downlink_interference = default(false);
        // if true, enables the interference computation for UL connections -->
        bool uplink_interference = default(false);
	// if true, enables the interference computation for D2D connections -->
        bool d2d_interference = default(true);

        bool enable_extCell_los = default(true);

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

