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
import inet.node.inet.Router;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.mec.MECHost;
import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;
import simu5g.nodes.backgroundCell.BackgroundCell;
//import simu5g.nodes.trafficLight.TrafficLight;
import simu5g.world.radio.LteChannelControl;

//
// UrbanNetwork
//
network UrbanNetwork
{
    parameters:
        int numUe = default(1);
        int numBgCells = default(0);
        @display("i=block/network2;bgb=3538.7122,3322.424");
    submodules:

        channelControl: LteChannelControl {
            @display("p=114.647995,32.596;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=114.647995,82.052;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=115.771996,143.872");
        }
        binder: Binder {
            @display("p=115.771996,276.504;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=114.647995,337.19998;is=s");
        }

//        //# traffic lights
//        trafficLight1: TrafficLight {
//            @display("p=312.644,2867.0122");
//        }
//        trafficLight2: TrafficLight {
//            @display("p=1430.18,3033.312");
//        }
//        trafficLight3: TrafficLight {
//            @display("p=2354.808,2906.924");
//        }
//        trafficLight4: TrafficLight {
//            @display("p=818.19604,3079.8762");
//        }
//        trafficLight5: TrafficLight {
//            @display("p=1849.2561,2906.924");
//        }
//        trafficLight6: TrafficLight {
//            @display("p=2760.58,2733.9722");
//        }

        //# CN modules

        upf: Upf {
            @display("p=997.80005,199.56001");
        }
        iUpf: Upf {
            @display("p=997.80005,772.6554");
        }

        //# MEC modules
        mecHost: MECHost {
            @display("p=1744.8708,772.6554");
        }
        mecOrchestrator: MecOrchestrator {
            @display("p=2353.7847,199.56001");
        }
        ualcmp: UALCMP {
            @display("p=1744.8708,199.56001");
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
            @display("p=1775.5724,2246.3293;is=l");
        }

        ue[numUe]: NRUe {
            @display("p=496.34155,2026.3016");
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
