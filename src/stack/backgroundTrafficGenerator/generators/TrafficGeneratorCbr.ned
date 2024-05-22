
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
// The TrafficGeneratorCbr module generates Constant Bit Rate (CBR) traffic 
// for background users. It extends the TrafficGeneratorBase and provides 
// customizable parameters such as packet size and inter-arrival periods 
// for both Downlink (DL) and Uplink (UL) traffic.
// 
simple TrafficGeneratorCbr extends TrafficGeneratorBase
{
    parameters:
        @display("i=block/source");
        @class("TrafficGeneratorCbr");
}
