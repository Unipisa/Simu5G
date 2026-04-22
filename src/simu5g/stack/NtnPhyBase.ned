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

package simu5g.stack;

simple NtnPhyBase
{
    parameters:
        @display("i=block/wrxtx;bgb=704,443;bgl=3");
        string binderModule = default("binder");
        string channelModelModule = default("^.channelModel[0]");
        string linkType @enum("feeder","service");
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @loose @directIn;
}
