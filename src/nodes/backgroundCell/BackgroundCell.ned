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

package simu5g.nodes.backgroundCell;

import inet.mobility.static.StationaryMobility;
import simu5g.stack.backgroundTrafficGenerator.BackgroundTrafficGenerator;

//
// BackgroundCell Module
//
// This background cell implements only one carrier.
// If you want to simulate multiple carriers, deploy multiple background cells in the same position
// and change carrierFrequency parameter
module BackgroundCell
{
    parameters:
        @display("i=device/antennatower;bgb=430,163");

        string bgTrafficManagerType = default("BackgroundCellTrafficManager");

    submodules:
        // mobility module for the background base station
        mobility: StationaryMobility {
            @display("p=100,38;is=s");
        }

        // channel model for the background cell
        bgChannelModel: BackgroundCellChannelModel {
            @display("p=100,105;is=s");
        }

        // perform the scheduling for the BackgroundCell
        bgScheduler: BackgroundScheduler {
            @display("p=222,38,row");
        }

        // traffic generator for the background UEs within the BackgroundCell
        bgTrafficGenerator: BackgroundTrafficGenerator {
            @display("p=347,38,row");
            bgTrafficManagerType = parent.bgTrafficManagerType;
        }
}


