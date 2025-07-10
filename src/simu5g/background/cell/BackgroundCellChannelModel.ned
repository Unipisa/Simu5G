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

package simu5g.background.cell;

//
// This module configures the channel parameters associated with a ~BackgroundCell. It is used
// to compute the channel quality for background UEs (refer to the ~BackgroundUe module), which in
// turn is essential for deriving the correct allocation of time-frequency resources via the
// Adaptive Modulation and Coding mechanism.
//
simple BackgroundCellChannelModel
{
    parameters:
        @display("i=block/control");
        @class("BackgroundCellChannelModel");

        // modules
        string binderModule = default("binder");

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

        double streetWidth @unit(m) = default(20m); //TODO width
        double ueHeight @unit(m) = default(1.5m);
        bool tolerateMaxDistViolation = default(false);

        double correlationDistance @unit(m) = default(50m);

        // Antenna Gain of UE --> wrt an isotropic antenna
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
        // Fading type (JAKES or RAYLEIGH) -->
        string fadingType @enum(RAYLEIGH,JAKES) = default("JAKES");
        // If jakes fading this parameter specify the number of path (tap channel) -->
        int numFadingPaths = default(6);
        double delayRms @unit(s) = default(363ns);

        // if true, enables the inter-cell interference computation for UL and DL connections from background cells -->
        bool bgCellInterference = default(true);
        // if true, enables the inter-cell interference computation for DL connections -->
        bool downlinkInterference = default(true);
        // if true, enables the interference computation for UL connections -->
        bool uplinkInterference = default(true);
}
