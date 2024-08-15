//
//                           Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.emulation.mec.extMecApp;

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


network ExtMecAppExample
{
    parameters:

        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=805,500");

    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=690,104;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=556,34");
        }
        channelControl: LteChannelControl {
            @display("p=690,24;is=s");
        }
        binder: Binder {
            @display("p=474,34;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=368,34;is=s");
        }
        upf: Upf {
            @display("p=420,259");
        }
        iUpf: Upf {
            @display("p=277,259");
        }
        gNodeB1: gNodeB {
            @display("p=137,260;is=vl");
        }

        //# MEC modules
        mecHost1: MECHost {
            @display("p=277,158;is=l");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=520,158;is=l");
        }

        ualcmp: UALCMP {
            @display("p=420,157;i=device/smallrouter");
        }

        ue: NRUe {
            @display("p=51,259");
        }

    connections allowunconnected:

        gNodeB1.ppp <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> upf.pppg++;

	    //#	mec
        mecHost1.ppp++ <--> Eth10G <--> iUpf.pppg++;
        upf.filterGate <--> Eth10G <--> ualcmp.ppp++;

        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;
}
