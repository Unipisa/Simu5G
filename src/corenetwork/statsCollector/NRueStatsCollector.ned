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
// This is the statistic collector of an eNodeB. It stores all the attributes of the RNI
// service Layer2Meaurements resource. The RNI service will call its methods in order to
// responds to requests

//

module NRueStatsCollector extends UeStatsCollector
{
    parameters:
        collectorType = default("NRueStatsCollector");
}










