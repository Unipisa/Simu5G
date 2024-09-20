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

package simu5g.stack.backgroundTrafficGenerator.generators;

//
// This module specializes the ~TrafficGeneratorBase module to provide a
// Constant Bit Rate (CBR) background traffic generator. This generator
// produces fixed-size packets at regular intervals in either downlink
// or uplink bands, or both.
//
simple TrafficGeneratorCbr extends TrafficGeneratorBase
{
    parameters:
        @display("i=block/source");
        @class("TrafficGeneratorCbr");
}
