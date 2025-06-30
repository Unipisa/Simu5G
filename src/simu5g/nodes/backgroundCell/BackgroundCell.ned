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
// This module implements a lightweight model for a 4G eNodeB or 5G gNodeB. It is used to generate
// configurable inter-cell interference in the simulated network. Compared to the ~eNodeB and ~gNodeB
// modules, this module occupies time-frequency resources in both the uplink and downlink bandwidth
// without simulating the actual transmission or reception of data packets to or from User Equipments (UEs).
// Therefore, it is significantly more efficient from a simulation speed perspective.
// To achieve this, it includes a ~BackgroundTrafficGenerator to customize the level of background
// traffic to be simulated, a ~BackgroundChannelModel to configure channel parameters, and a
// ~BackgroundScheduler that manages the allocation of time-frequency resources based on the traffic
// generator.
// Note that this module can generate interference on only one Component Carrier, as configured
// in the ~BackgroundScheduler module. To produce interference across multiple carriers, it is
// possible to use multiple instances of this module, each configured with the desired carrier frequency.
//
// For further details, refer to:
//     G. Nardini, G. Stea, A. Virdis, "Scalable Real-time Emulation of 5G Networks with Simu5G",
//     IEEE Access, vol. 9, pp. 148504-148520, 2021
//
module BackgroundCell
{
    parameters:
        @display("i=device/antennatower;bgb=430,163");

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
            trafficManagerModule = "^.bgTrafficGenerator.manager";
        }

        // traffic generator for the background UEs within the BackgroundCell
        bgTrafficGenerator: BackgroundTrafficGenerator {
            @display("p=347,38,row");
            manager.typename = default("BackgroundCellTrafficManager");
        }
}


