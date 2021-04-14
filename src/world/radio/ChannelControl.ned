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
// ~ChannelControl has exactly one instance in every network model that
// contains mobile nodes. 
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
        string propagationModel @enum("FreeSpaceModel","TwoRayGroundModel","RiceModel","RayleighModel","NakagamiModel","LogNormalShadowingModel") = default("FreeSpaceModel");
        @display("i=misc/sun");
        @labels(node);
}