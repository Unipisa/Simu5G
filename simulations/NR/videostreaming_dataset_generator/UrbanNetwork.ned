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

package simu5g.simulations.NR.videostreaming_dataset_generator;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.mobility.trafficLightMobility.TrafficLight;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.Upf;
import simu5g.nodes.backgroundCell.BackgroundCell;
import simu5g.nodes.mec.MECHost;
import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;
import simu5g.world.radio.LteChannelControl;


//
// UrbanNetwork
//
network UrbanNetwork
{
    parameters:
        int numUe = default(1);
        int numBgCells = default(0);
        @display("i=block/network2;bgb=2000,2000;bgi=intersections");
    submodules:

        channelControl: LteChannelControl {
            @display("p=114,32;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=114,82;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=115,143;is=s");
        }
        binder: Binder {
            @display("p=115,242;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=114,337;is=s");
        }

        //# traffic lights
        tl1: TrafficLight {
            @display("p=500,2026");
        }
        tl2: TrafficLight {
            @display("p=1000,2026");
        }
        tl3: TrafficLight {
            @display("p=1500,2026");
        }
        tl4: TrafficLight {
            @display("p=500,2026");
        }
        tl5: TrafficLight {
            @display("p=1000,2026");
        }
        tl6: TrafficLight {
            @display("p=1500,2026");
        }

        //# CN modules

        upf: Upf {
            @display("p=1000,1050");
        }
        iUpf: Upf {
            @display("p=1000,1200");
        }

        //# MEC modules
        mecHost: MECHost {
            @display("p=1150,1200");
        }
        mecOrchestrator: MecOrchestrator {
            @display("p=1300,1050");
        }
        ualcmp: UALCMP {
            @display("p=1150,1050");
        }

        //# RAN modules

        gnb1: gNodeB {
            @display("p=500,1500;is=vl");
        }
        gnb2: gNodeB {
            @display("p=1000,1500;is=vl");
        }
        gnb3: gNodeB {
            @display("p=1500,1500;is=vl");
        }

        bgCell[numBgCells]: BackgroundCell {
            @display("p=1971,1498;is=l");
        }

        ue[numUe]: NRUe {
            @display("p=200,2026");
        }

    connections:

        //# CN connections

        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gnb1.ppp;
        iUpf.pppg++ <--> Eth10G <--> gnb2.ppp;
        iUpf.pppg++ <--> Eth10G <--> gnb3.ppp;

        //# MEC connections

        ualcmp.ppp++ <--> Eth10G <--> upf.filterGate;
        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;
        mecHost.ppp++ <--> Eth10G <--> iUpf.pppg++;

        //# X2 connections
        gnb1.x2++ <--> Eth10G <--> gnb2.x2++;
        gnb1.x2++ <--> Eth10G <--> gnb3.x2++;
        gnb2.x2++ <--> Eth10G <--> gnb3.x2++;
}
