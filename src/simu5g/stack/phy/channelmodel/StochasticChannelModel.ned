//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.phy.channelmodel;

//
// The full PHY link model: everything between a transmitted air frame and the
// decision on whether it was received. The channel impairments are modeled
// statistically -- drawn from the distributions of a 3GPP propagation study,
// rather than computed from the geometry of an actual environment -- which is
// what sets this model apart from ~IdealChannelModel, where there are no
// impairments at all, only a configured packet error rate.
//
// It covers:
// - path loss per deployment scenario, LOS/NLOS state and log-normal shadowing,
//   with the formulas of the 3GPP propagation study selected by pathLossType
//   (TR 36.814, TR 36.873 or TR 38.901);
// - multipath fading, Jakes or Rayleigh;
// - the antenna pattern attenuation and the link budget (antenna gains, cable
//   loss, noise figures, thermal noise);
// - interference from other cells: downlink, uplink, external cells and
//   background cells;
// - the assembly of all of the above into a per-band SINR, and the mapping of
//   that SINR onto a block error probability that decides reception.
//
// Only the path loss, LOS probability, shadowing and angular attenuation come
// from the selected propagation study; everything else is the same whichever
// study is selected. The ~Tr36873ChannelModel and ~Tr38901ChannelModel presets
// differ from this type only in their pathLossType default.
//
simple StochasticChannelModel extends ChannelModelBase
{
    parameters:
        @class("StochasticChannelModel");

        string pathLossType @enum("Tr36814","Tr36873","Tr38901") = default("Tr36814"); // Which 3GPP propagation study supplies the path loss formulas

        // Enable/disable shadowing
        bool shadowing = default(true);

        // Path loss scenario, in ITU terminology
        string scenario @enum(INDOOR_HOTSPOT,URBAN_MICROCELL,URBAN_MACROCELL,RURAL_MACROCELL,SUBURBAN_MACROCELL,UNKNOWN_SCENARIO) = default("URBAN_MACROCELL");

        // eNodeB height
        double nodebHeight @unit(m) = default(25m);
        // Building height
        double buildingHeight @unit(m) = default(20m);
        // Determines if the UE is inside a building
        bool insideBuilding = default(false);
        // Enable the high-loss building penetration model (table 7.4.3-2 in TR 38.901); only used with pathLossType="Tr38901"
        bool useBuildingPenetrationHighLossModel = default(false);

        double streetWidth @unit(m) = default(20m);
        double ueHeight @unit(m) = default(1.5m);
        bool tolerateMaxDistViolation = default(false);

        double correlationDistance @unit(m) = default(50m);

        // Target BLER used to compute feedback
        double targetBler = default(0.01);
        // Factor the error probability is multiplied by for each HARQ retransmission
        double harqReduction = default(0.2);

        // Antenna gain of the UE
        double antennaGainUe @unit(dBi) = default(0dBi);
        // Antenna gain of the eNodeB
        double antennGainEnB @unit(dBi) = default(18dBi);
        // Thermal noise for 10 MHz of bandwidth
        double thermalNoise @unit(dBm) = default(-104.5dBm);
        // UE noise figure
        double ueNoiseFigure @unit(dBm) = default(7dBm);
        // eNodeB noise figure
        double bsNoiseFigure @unit(dBm) = default(5dBm);
        // Cable loss
        double cableLoss @unit(dB) = default(2dB);

        // Whether LOS/NLOS is drawn per link from the scenario's LOS probability; when false, it is fixed to fixedLos
        bool dynamicLos = default(false);
        // LOS state of every link when dynamicLos is false: true = LOS, false = NLOS
        bool fixedLos = default(false);
        // Enable/disable fading
        bool fading = default(true);
        // Fading type (JAKES or RAYLEIGH)
        string fadingType @enum(RAYLEIGH,JAKES) = default("JAKES");
        // If Jakes fading, this parameter specifies the number of paths (tap channel)
        int numFadingPaths = default(6);

        double delayRms @unit(s) = default(363ns);

        // Whether interference from background cells is included in the SINR, in both UL and DL
        bool bgCellInterference = default(true);
        // Whether interference from external cells is included in the DL SINR (TODO possibly obsolete)
        bool extCellInterference = default(true);
        // Whether interference from other base stations is included in the DL SINR
        bool downlinkInterference = default(false);
        // Whether interference from the UEs of other cells is included in the UL SINR
        bool uplinkInterference = default(false);

        bool enableExtCellLos = default(true);

        // Collection of SINR statistics can be disabled because it might be quite time-consuming
        bool collectSinrStatistics = default(true);

        // Statistics
        @signal[rcvdSinrDl];
        @statistic[rcvdSinrDl](title="SINR measured at packet reception, DL"; unit="dB"; source="rcvdSinrDl"; record=mean,vector);
        @signal[rcvdSinrUl];
        @statistic[rcvdSinrUl](title="SINR measured at packet reception, UL"; unit="dB"; source="rcvdSinrUl"; record=mean,vector);

        @signal[measuredSinrDl];
        @statistic[measuredSinrDl](title="SINR measured at feedback computation, DL"; unit="dB"; source="measuredSinrDl"; record=mean,vector);
        @signal[measuredSinrUl];
        @statistic[measuredSinrUl](title="SINR measured at feedback computation, UL"; unit="dB"; source="measuredSinrUl"; record=mean,vector);
}

