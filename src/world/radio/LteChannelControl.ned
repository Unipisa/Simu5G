
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

package simu5g.world.radio;

//
// The LteChannelControl module is responsible for managing the LTE channel 
// model within the Simu5G environment. It extends the functionality of the
// ChannelControl module to include LTE-specific channel characteristics such as
// interference calculation based on LTE parameters (e.g., carrier frequency, 
// transmission power, path loss coefficient). This module helps in simulating 
// the physical layer behavior accurately, including signal attenuation and 
// interference among LTE nodes.
//
simple LteChannelControl extends ChannelControl
{
    parameters:
        @display("i=misc/sun");
        @labels(node);
        @class(LteChannelControl);
}
