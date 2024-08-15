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

package simu5g.simulations.NR.mec.singleMecHost;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.Upf;
import simu5g.nodes.mec.MECHost;
import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;
import simu5g.world.radio.LteChannelControl;


network singleMecHost
{
    parameters:
        @display("i=block/network2;bgb=811,507;bgi=background/pisa");

        int numUes = default(0);

        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)

    submodules:

        routingRecorder: RoutingTableRecorder {
            @display("p=690,104;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=690,184;is=s");
        }
        channelControl: LteChannelControl {
            @display("p=690,24;is=s");
        }
        binder: Binder {
            @display("p=690,264;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=691,342;is=s");
        }
        upf: Upf {
            @display("p=545,250");
        }
        iUpf: Upf {
            @display("p=352,250");
        }
        gNodeB1: gNodeB {
            @display("p=140,250;is=vl");
        }
        ue[numUes]: NRUe {
            @display("p=237,310");
        }
        //# MEC module
        mecHost: MECHost {
            @display("p=352,145;is=l");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=544,63;is=l");
        }

        ualcmp: UALCMP {
            @display("p=545,158;i=device/smallrouter");
        }


    connections allowunconnected:

        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gNodeB1.ppp;
        ualcmp.ppp++ <--> Eth10G <--> upf.filterGate;

        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;

        mecHost.ppp++ <--> Eth10G <--> iUpf.pppg++;
}
