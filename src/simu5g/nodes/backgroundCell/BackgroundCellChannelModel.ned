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
        double nodeb_height @unit(m) = default(25m);
        // Building height -->
        double building_height @unit(m) = default(20m);
        // determines if the UE is inside a building
        bool inside_building = default(false);

        double street_wide @unit(m) = default(20m); //TODO width
        double ue_height @unit(m) = default(1.5m);
        bool tolerateMaxDistViolation = default(false);

        double correlation_distance @unit(m) = default(50m);

        // Antenna Gain of UE --> wrt an isotropic antenna
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
        // Fading type (JAKES or RAYLEIGH) -->
        string fading_type @enum(RAYLEIGH,JAKES) = default("JAKES");
        // If jakes fading this parameter specify the number of path (tap channel) -->
        int fading_paths = default(6);
        double delay_rms @unit(s) = default(363ns);

        // if true, enables the inter-cell interference computation for UL and DL connections from background cells -->
        bool bgCell_interference = default(true);
        // if true, enables the inter-cell interference computation for DL connections -->
        bool downlink_interference = default(true);
        // if true, enables the interference computation for UL connections -->
        bool uplink_interference = default(true);
}
