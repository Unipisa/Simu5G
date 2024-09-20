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

//
// This module provides the interface for modules implementing the model of
// the radio channel. Such modules are responsible for providing functions to compute
// the Signal to Interference-plus-Noise Ratio (SINR) of received frames, taking into
// account channel effects such as propagation loss, fading/shadowing, and inter-cell
// interference.
//
moduleinterface ILteChannelModel
{
    @display("i=block/classifier2");
}

