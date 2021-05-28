//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.simulations.NR.mec_rest.meco_multiMecHost;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import lte.world.radio.LteChannelControl;
import lte.nodes.binder.LteBinder;
import lte.common.carrierAggregation.CarrierAggregation;
import lte.nodes.Upf;
import lte.nodes.NR.gNodeB;
import lte.nodes.NR.NRUe;
import lte.nodes.mec.MEHost;

import inet.node.inet.Router;


import lte.nodes.mec.MECOrchestrator.MecOrchestrator;
import lte.nodes.mec.LCMProxy.LCMProxy;



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
        binder: LteBinder {
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
        meHost1: MEHost {
            @display("p=390.3425,385.28625;is=l");
        }

        meHost2: MEHost {
            @display("p=513.715,385.28625;is=l");
        }


        mecOrchestrator: MecOrchestrator {
            @display("p=556.1875,177.98;is=l");
        }

        lcmProxy: LCMProxy {
            @display("p=391.35376,176.96875;i=device/smallrouter");
        }


    connections allowunconnected:

        upf.pppg++ <--> Eth10G <--> gNodeB1.ppp;

        router.pppg++ <--> Eth10G <--> upf.filterGate;

        router.pppg++ <--> Eth10G <--> lcmProxy.ppp++;

        lcmProxy.toMecOrchestrator --> mecOrchestrator.fromLcmProxy;
        lcmProxy.fromMecOrchestrator <-- mecOrchestrator.toLcmProxy;

        //#	mec
        // direct connection between gNodeB1 and meHost
        meHost1.ppp++ <--> Eth10G <--> router.pppg++;
        meHost2.ppp++ <--> Eth10G <--> router.pppg++;

}
