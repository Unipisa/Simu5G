//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.simulations.NR.mec_rest.warningApp;

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

network MecSingleCell
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
            @display("p=506.46,249.78;is=l");
        }
        gNodeB1: gNodeB {
            @display("p=140,250;is=vl");
        }
        car[numCars]: NRUe {
            @display("p=300,300");
        }
        //# MEC module
        meHost: MEHost {
            @display("p=404.34,144.9;is=l");
        }


    connections allowunconnected:

        upf.pppg++ <--> Eth10G <--> gNodeB1.ppp;

        //#	mec
        // direct connection between gNodeB1 and meHost
        meHost.pppENB++ <--> Eth10G <--> gNodeB1.pppMEHost;
}
