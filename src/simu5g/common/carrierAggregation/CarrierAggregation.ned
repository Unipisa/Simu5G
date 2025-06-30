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
// This module implements the Carrier Aggregation (CA) functionality used at the
// Medium Access Control (MAC) layer of e/gNodeBs and UEs. CA allows multiple frequency bands -
// referred to as Component Carriers (CCs) - to be aggregated in order to increase the overall
// available bandwidth of the system. Each CC is implemented as a submodule and can be
// configured independently from each other. e/gNodeB and UE can be configured to use
// one or more CCs among those configured in this module.
// There must be one (and only one) instance of this module in the network.
//
module CarrierAggregation
{
    parameters:
        @display("i=block/control");
        int numComponentCarriers = default(1);

    submodules:
        componentCarrier[numComponentCarriers]: ComponentCarrier;
}

