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

import inet.mobility.static.StationaryMobility;
import simu5g.stack.backgroundTrafficGenerator.BackgroundTrafficGenerator;

// 
// BackgroundCell Module
//
// This background cell implements only one carrier.
// If you want to simulate multiple carriers, deploy multiple background cells in the same position
// and change carrierFrequency parameter  
module BackgroundCell
{
    parameters:
        @display("i=device/antennatower;bgb=430,163");

        string bgTrafficManagerType = default("BackgroundCellTrafficManager");

    submodules:
        // mobility module for the background base station
        mobility: StationaryMobility {
            @display("p=100,38;is=s");
        }

        // channel model for the background cell
        bgChannelModel: BackgroundCellChannelModel {
            @display("p=100,105;is=s");
        }

        // perform the scheduling for the BackgroundCell
        bgScheduler: BackgroundScheduler {
            @display("p=222,38,row");
        }

        // traffic generator for the background UEs within the BackgroundCell
        bgTrafficGenerator: BackgroundTrafficGenerator {
            @display("p=347,38,row");
            bgTrafficManagerType = parent.bgTrafficManagerType;
        }
}


simple BackgroundScheduler
{
    parameters:
        @display("i=block/control");
        @class("BackgroundScheduler");

        // transmission power
        double txPower = default(46);

        // transmission direction
        string txDirection = default("OMNI");

        // transmission angle
        double txAngle = default(0);

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
        @statistic[bgAvgServedBlocksDl](title="Average DL served blocks for background cells"; unit=""; source="bgAvgServedBlocksDl"; record=mean, vector);
        @signal[bgAvgServedBlocksUl];
        @statistic[bgAvgServedBlocksUl](title="Average UL served blocks for background cells"; unit=""; source="bgAvgServedBlocksUl"; record=mean, vector);
}

simple BackgroundCellChannelModel
{
    parameters:
        @display("i=block/control");
        @class("BackgroundCellChannelModel");

        // Enable/disable shadowing        
        bool shadowing = default(true);

        // Pathloss scenario from ITU -->   
        string scenario = default("URBAN_MACROCELL");

        // eNodeB height -->
        double nodeb_height = default(25);
        // Building height -->
        double building_height = default(20);
        // determines if the UE is inside a building
        bool inside_building = default(false);

        double street_wide = default(20);
        double ue_height = default(1.5);
        bool tolerateMaxDistViolation = default(false);

        double correlation_distance = default(50);

        // Antenna Gain of UE -->
        double antennaGainUe = default(0);
        // Antenna Gain of eNodeB -->
        double antennGainEnB = default(18);
        // Antenna Gain of Micro node -->
        double antennGainMicro = default(5);
        // Thermal Noise for 10 MHz of Bandwidth -->
        double thermalNoise = default(-104.5);
        // Ue noise figure -->
        double ue_noise_figure = default(7);
        // eNodeB noise figure -->
        double bs_noise_figure = default(5);
        // Cable Loss -->
        double cable_loss = default(2);

        // If true enable the possibility to switch dinamically the LOS/NLOS pathloss computation -->
        bool dynamic_los = default(false);
        // If dynamic-los is false this parameter, if true, compute LOS pathloss otherwise compute NLOS pathloss -->
        bool fixed_los = default(false);
        // Enable/disable fading -->  
        bool fading = default(true);
        // Fading type (JAKES or RAYGHLEY) -->  
        string fading_type = default("JAKES");
        // If jakes fading this parameter specify the number of path (tap channel) -->  
        int fading_paths = default(6);
        double delay_rms = default(363e-9);
        
        // if true, enables the inter-cell interference computation for UL and DL connections from background cells -->  
        bool bgCell_interference = default(true);
        // if true, enables the inter-cell interference computation for DL connections -->  
        bool downlink_interference = default(true);
        // if true, enables the interference computation for UL connections -->
        bool uplink_interference = default(true);
}
