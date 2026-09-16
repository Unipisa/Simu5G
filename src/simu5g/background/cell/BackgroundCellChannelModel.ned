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

package simu5g.background.cell;

//
// Configures the channel parameters associated with a ~BackgroundCell. It is used
// to compute the channel quality for background UEs (refer to the ~BackgroundUe module), which in
// turn is essential for deriving the correct allocation of time-frequency resources via the
// Adaptive Modulation and Coding mechanism.
//
simple BackgroundCellChannelModel
{
    parameters:
        @display("i=block/control");
        @class("BackgroundCellChannelModel");

        // Module references
        string binderModule = default("binder");

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

        double streetWidth @unit(m) = default(20m); // TODO width
        double ueHeight @unit(m) = default(1.5m);
        bool tolerateMaxDistViolation = default(false);

        double correlationDistance @unit(m) = default(50m);

        // Antenna gain of the UE, relative to an isotropic antenna
        double antennaGainUe @unit(dBi) = default(0dBi);
        // Antenna gain of the eNodeB
        double antennGainEnB @unit(dBi) = default(18dBi);
        // Antenna gain of the micro node
        double antennGainMicro @unit(dBi) = default(5dBi);
        // Thermal noise for 10 MHz of bandwidth
        double thermalNoise @unit(dBm) = default(-104.5dBm);
        // UE noise figure
        double ueNoiseFigure @unit(dBm) = default(7dBm);
        // eNodeB noise figure
        double bsNoiseFigure @unit(dBm) = default(5dBm);
        // Cable loss
        double cableLoss @unit(dB) = default(2dB);

        // If true, LOS/NLOS is drawn per link from the scenario's LOS probability; if false, it is fixed to fixedLos
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

        // If true, enables the inter-cell interference computation for UL and DL connections from background cells
        bool bgCellInterference = default(true);
        // If true, enables the inter-cell interference computation for DL connections
        bool downlinkInterference = default(true);
        // If true, enables the interference computation for UL connections
        bool uplinkInterference = default(true);
}
