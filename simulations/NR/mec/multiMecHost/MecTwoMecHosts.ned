//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.simulations.NR.mec.multiMecHost;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import simu5g.world.radio.LteChannelControl;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.mec.MECHost;

import inet.node.inet.Router;


import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;



network MecTwoMecHosts
{
    parameters:

        int numCars = default(0);

        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=805.92,499.56");

    submodules:

        routingRecorder: RoutingTableRecorder {
            @display("p=690,104.88;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=690,184.92");
        }
        channelControl: LteChannelControl {
            @display("p=690,24.84;is=s");
        }
        binder: Binder {
            @display("p=690,264.96;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=691.38,342.24;is=s");
        }
        upf: Upf {
            @display("p=284.16125,259.89127;is=l");
        }
        gNodeB1: gNodeB {
            @display("p=137.53,259.89127;is=vl");
        }
        car[numCars]: NRUe {
            @display("p=145.62,385.28625");
        }

        router: Router {
            @display("p=391.35376,259.89127");
        }

        //# MEC modules
        mecHost1: MECHost {
            @display("p=390.3425,385.28625;is=l");
        }

        mecHost2: MECHost {
            @display("p=513.715,385.28625;is=l");
        }


        mecOrchestrator: MecOrchestrator {
            @display("p=556.1875,177.98;is=l");
        }

        ualcmp: UALCMP {
            @display("p=391.35376,176.96875;i=device/smallrouter");
        }


    connections allowunconnected:

        upf.pppg++ <--> Eth10G <--> gNodeB1.ppp;

        router.pppg++ <--> Eth10G <--> upf.filterGate;

        router.pppg++ <--> Eth10G <--> ualcmp.ppp++;

        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;

        //#	mec
        // direct connection between gNodeB1 and meHost
        mecHost1.ppp++ <--> Eth10G <--> router.pppg++;
        mecHost2.ppp++ <--> Eth10G <--> router.pppg++;

}
