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
package simu5g.simulations.NR.mec.platooning;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.mec.MECHost;
import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.backgroundCell.BackgroundCell;
import simu5g.world.radio.LteChannelControl;
import inet.node.inet.StandardHost;


// MultiOperator
//
// This network showcases a scenario with two 5G networks (CN+RAN), each of them 
// including three gNBs and one MEC host.
// Operator B's MEC system can be disabled using the 'enableMecOperatorB' parameter
//
// Also see 'demo.xml' for the correct configuration of network interfaces 
//

network MultiOperator
{
    parameters:
        double routersDelay @unit(s) = default(0.1ms);
        int numUe_A = default(1);
        int numUe_B = default(1);
        int numBgCells = default(0);
        bool enableMecOperatorB = default(true);
        @display("i=block/network2;bgb=1912.7875,795.925");//;bgi=background/pisa_large");

        int linkLength @unit(m) = default(0m); // it is in ms!!!!
    submodules:
        channelControl: LteChannelControl {
            @display("p=192.5625,51.350002;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=192.5625,102.700005;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=192.5625,151.4825;is=s");
        }
        binder: Binder {
            @display("p=192.5625,202.8325;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=192.5625,284.9925;is=s");
        }
        router2: Router {
            @display("p=1160.51,74.457504");
        }
        router1: Router {
            @display("p=908.895,74.457504");
        }
        upf_A: Upf {
            @display("p=911.4625,174.59");
        }
        upf_B: Upf {
            @display("p=1160.51,174.59");
        }
        iUpf_A: Upf {
            @display("p=908.895,282.42502");
        }
        iUpf_B: Upf {
            @display("p=1160.51,282.42502");
        }
        gnb_A1: gNodeB {
            @display("p=274.365,454.54498;is=vl");
        }
        gnb_A2: gNodeB {
            @display("p=797.15125,452.60248;is=vl");
        }
        gnb_A3: gNodeB {
            @display("p=1245.6763,452.60248;is=vl");
        }
        gnb_A4: gNodeB {
            @display("p=1745.6763,452.60248;is=vl");
        }
        gnb_B1: gNodeB {
            @display("p=330.33,514.605;is=vl");
        }
        gnb_B2: gNodeB {
            @display("p=848.12,513.765;is=vl");
        }
        gnb_B3: gNodeB {
            @display("p=1304.7999,513.765;is=vl");
        }
        gnb_B4: gNodeB {
            @display("p=1804.7999,513.765;is=vl");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=138.635,629.97375;is=vl");
        }
        ue_A[numUe_A]: NRUe {
            @display("p=717.64,691.1362");
        }
        ue_B[numUe_B]: NRUe {
            @display("p=937.825,691.1362");
        }

        //# MEC modules
        mecHost_A: MECHost {
            @display("p=559.715,282.42502");
        }
        mecHost_B: MECHost if enableMecOperatorB {
            @display("p=1532.7975,282.42502");
        }
        mecOrchestrator_A: MecOrchestrator {
            @display("p=559.715,74.457504");
        }
        mecOrchestrator_B: MecOrchestrator if enableMecOperatorB {
            @display("p=1532.7975,74.457504");
        }
        ualcmp_A: UALCMP {
            @display("p=775.385,74.457504");
        }
        ualcmp_B: UALCMP if enableMecOperatorB {
            @display("p=1311.9926,74.457504");
        }


    connections:

        //# 5G Core Network connections
        upf_A.pppg++ <--> Eth10G {  length = 300m; } <--> iUpf_A.pppg++;
        iUpf_A.pppg++ <--> Eth10G {  length = 300m; } <--> mecHost_A.ppp++;
        iUpf_A.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_A1.ppp;
        iUpf_A.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_A2.ppp;
        iUpf_A.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_A3.ppp;
        iUpf_A.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_A4.ppp;


        upf_B.pppg++ <--> Eth10G {  length = 300m; } <--> iUpf_B.pppg++;
        iUpf_B.pppg++ <--> Eth10G {  length = 300m; } <--> mecHost_B.ppp++ if enableMecOperatorB;
        iUpf_B.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_B1.ppp;
        iUpf_B.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_B2.ppp;
        iUpf_B.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_B3.ppp;
        iUpf_B.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_B4.ppp;



        //# MEC-related connections
        router1.pppg++ <--> Eth10G {  length = 300m; } <--> upf_A.filterGate;
        ualcmp_A.ppp++ <--> Eth10G {  length = 300m; } <--> router1.pppg++;
        ualcmp_A.toMecOrchestrator --> mecOrchestrator_A.fromUALCMP;
        ualcmp_A.fromMecOrchestrator <-- mecOrchestrator_A.toUALCMP;

        router2.pppg++ <--> Eth10G {  length = 300m; } <--> upf_B.filterGate;
        ualcmp_B.ppp++ <--> Eth10G {  length = 300m; } <--> router2.pppg++ if enableMecOperatorB;
        ualcmp_B.toMecOrchestrator --> mecOrchestrator_B.fromUALCMP if enableMecOperatorB;
        ualcmp_B.fromMecOrchestrator <-- mecOrchestrator_B.toUALCMP if enableMecOperatorB;

        //# X2 connections
        gnb_A1.x2++ <--> Eth10G {  length = 500m; } <--> gnb_A2.x2++;
        gnb_A2.x2++ <--> Eth10G {  length = 500m; } <--> gnb_A3.x2++;
        gnb_A3.x2++ <--> Eth10G {  length = 500m; } <--> gnb_A4.x2++;

        gnb_B1.x2++ <--> Eth10G {  length = 500m; } <--> gnb_B2.x2++;
        gnb_B2.x2++ <--> Eth10G {  length = 500m; } <--> gnb_B3.x2++;
        gnb_B3.x2++ <--> Eth10G {  length = 500m; } <--> gnb_B4.x2++;

        //# Routers connection
        router1.pppg++ <--> Eth10G {  length = parent.linkLength*2e5; } <--> router2.pppg++;
}
