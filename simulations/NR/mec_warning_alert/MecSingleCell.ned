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

package simu5g.simulations.NR.mec_warning_alert;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import simu5g.world.radio.LteChannelControl;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.mec.MEHost;

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
            config = xmldoc("demo.xml");
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
