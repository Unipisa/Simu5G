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
// A module implementing this interface is a utility module that is accessed by
// the ~RNIService of a ~MECHost module to retrieve radio network information related
// to the parent e/gNodeB module. The RNIS accesses this module via direct method calls.
// For example, see ~BaseStationStatsCollector or ~UeStatsCollector.
//
moduleinterface IStatsCollector
{
    string collectorType; // it can be: "eNBStatsCollector, gNBStatsCollector, ueStatsCollector, NRueStatsCollector
}

