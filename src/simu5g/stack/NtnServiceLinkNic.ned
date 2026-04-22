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

import simu5g.stack.phy.channelmodel.ILteChannelModel;

module NtnServiceLinkNic
{
    parameters:
        @display("i=block/ifcard;bgb=704,443;bgl=3");
        string channelModelType = default("NrChannelModel_3GPP38_901");
        int numCarriers = default(1);
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @directIn;
    submodules:
        phy: NtnPhyBase {
            parameters:
                @display("p=352,221");
                linkType = "service";
        }
        channelModel[numCarriers]: <channelModelType> like ILteChannelModel {
            @display("p=100,221");
        }
    connections:
        upperLayerIn --> phy.upperLayerIn;
        upperLayerOut <-- phy.upperLayerOut;
        radioIn --> phy.radioIn;
}
