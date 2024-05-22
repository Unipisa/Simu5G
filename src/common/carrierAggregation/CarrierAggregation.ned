
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

package simu5g.common.carrierAggregation;

//
// This module represents the carrier aggregation functionality used in gNodeBs and UEs.
// It allows the configuration of multiple component carriers (CCs) to aggregate bandwidth
// and improve the overall data rates and network performance.
//
module CarrierAggregation
{
    parameters:
        @display("i=block/control");
        int numComponentCarriers = default(1);

    submodules:
        componentCarrier[numComponentCarriers]: ComponentCarrier;
}

