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
// This module collects the radio network information from a 5G gNodeB, which can
// be accessed by the ~RNIService module of a ~MECHost. Currently, it exposes
// the same information as the ~BaseStationStatsCollector module.
//
module GNodeBStatsCollector extends BaseStationStatsCollector
{
    parameters:
        collectorType = default("gNBStatsCollector");
}

