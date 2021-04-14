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


moduleinterface LteChannelModelInterface
{
    @display("i=block/classifier2");
}

simple LteChannelModel like LteChannelModelInterface
{
    parameters:
    @display("i=block/classifier2");
    @class("LteChannelModel");
    
    // component carrier for this channel model
    int componentCarrierIndex = default(0);
}

simple LteRealisticChannelModel extends LteChannelModel
{
    parameters:
    @class("LteRealisticChannelModel");

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
    bool useTorus = default(false);

    double correlation_distance = default(50);

    // Target bler used to compute feedback -->
    double targetBler = default(0.01);
    // HARQ reduction -->
    double harqReduction = default(0.2);

    // Rank indicator tracefile -->
    double lambdaMinTh = default(0.02);
    double lambdaMaxTh = default(0.2);
    double lambdaRatioTh = default(20);

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

    // if true, enables the inter-cell interference computation for DL connections from external cells -->  
    bool extCell_interference = default(true);
    // if true, enables the inter-cell interference computation for DL connections -->  
    bool downlink_interference = default(false);
    // if true, enables the interference computation for UL connections -->
    bool uplink_interference = default(false);
	// if true, enables the interference computation for D2D connections -->  
    bool d2d_interference = default(true);
    
    bool enable_extCell_los = default(true);
    
    // read channel information from log file
    bool useRsrqFromLog = default(false);
    int rsrqShift = default(22);
    double rsrqScale = default(1.0);
    
    // statistics
    @signal[rcvdSinr];
    @statistic[rcvdSinr](title="SINR measured at packet reception"; unit="dB"; source="rcvdSinr"; record=mean,vector);
    
    // statistics
    @signal[measuredSinr];
    @statistic[measuredSinr](title="SINR measured at feedback computation"; unit="dB"; source="measuredSinr"; record=mean,vector);
    
    @signal[distance];
    @statistic[distance](title="distance between UE and serving eNB"; unit="meters"; source="distance"; record=mean,vector);
}

simple LteMagicChannelModel extends LteChannelModel
{
    parameters:
    @class("LteMagicChannelModel");
}

simple LteCrazyChannelModel extends LteChannelModel
{
    parameters:
    @class("LteCrazyChannelModel");
}

simple NRChannelModel extends LteRealisticChannelModel
{
    parameters:
    @class("NRChannelModel");
}