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
import simu5g.stack.phy.antennamodel.IAntennaModel;
import simu5g.stack.phy.NtnPhyBase;

module NtnServiceLinkNic
{
    parameters:
        @display("i=block/ifcard;bgb=507,396;bgl=3");
        string channelModelType = default("NtnChannelModel");
        string antennaModelType;
        int numCarriers = default(1);
        double feederLinkFrequencyOffset @unit(GHz) = default(25GHz);
        antennaModel.frequencyBand = default("S");

    gates:
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @directIn;
    submodules:
        phy: NtnPhyBase {
            parameters:
                @display("p=182,261");
                linkType = "service";
                feederLinkFrequencyOffset = parent.feederLinkFrequencyOffset;
        }
        channelModel[numCarriers]: <channelModelType> like ILteChannelModel {
            @display("p=182,145");
        }
        antennaModel: <antennaModelType> like IAntennaModel {
            @display("p=182,60");
        }
    connections:
        upperLayerIn --> phy.upperLayerIn;
        upperLayerOut <-- phy.upperLayerOut;
        radioIn --> phy.radioIn;
}
