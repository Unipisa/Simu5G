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


// NRChannelModel_3GPP38_901
//
// This channel model implements path loss, LOS probability, and shadowing according to
// the following specifications:
//     3GPP TR 38.901, "Study on channel model for frequencies from 0.5 to 100 GHz", v16.1.0, December 2019
//
// TODO: Implement Urban Macro only
//
simple NRChannelModel_3GPP38_901 extends NRChannelModel
{
    parameters:
        @class("NRChannelModel_3GPP38_901");

        bool useBuildingPenetrationLossHighLossModel = default(false);
}
