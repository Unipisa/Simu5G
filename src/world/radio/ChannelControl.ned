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
// Utility module that records the position of each radio module in the network.
// Exactly one instance of this module (or its derived extensions) must be present
// in the network. Note that channel parameters in this module are obsolete and
// are not utilized.
//
simple ChannelControl
{
    parameters:
        bool coreDebug = default(false); // debug switch for core framework
        double pMax @unit("mW") = default(20mW); // maximum sending power used for this network (in mW)
        double sat @unit("dBm") = default(-110dBm); // signal attenuation threshold (in dBm)
        double alpha = default(2); // path loss coefficient
        double carrierFrequency @unit("Hz") = default(2.4GHz); // base carrier frequency of all the channels (in Hz)
        int numChannels = default(1); // number of radio channels (frequencies)
        @display("i=misc/sun");
        @labels(node);
}
