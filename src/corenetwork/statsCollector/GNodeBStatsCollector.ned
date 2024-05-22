
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

package simu5g.corenetwork.statsCollector;
//
// This module represents the statistics collector for a gNodeB in a 5G NR network.
// It extends the BaseStationStatsCollector, integrating functionalities specific
// to gNodeB. The GNodeBStatsCollector is responsible for gathering and storing
// various performance metrics and statistics related to the gNodeB's operation,
// which are essential for analyzing network performance and troubleshooting.
//
module GNodeBStatsCollector extends BaseStationStatsCollector
{
    parameters:
        collectorType = default("gNBStatsCollector");
}

