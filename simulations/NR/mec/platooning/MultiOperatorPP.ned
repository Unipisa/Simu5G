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


// MultiOperatorPP
//
// This network showcases a scenario with four 5G networks (CN+RAN), each of them 
// including three gNBs and one MEC host.
// Operator B's MEC system can be disabled using the 'enableMecOperatorB' parameter
//
// Also see 'demo.xml' for the correct configuration of network interfaces 
//

network MultiOperatorPP
{
    parameters:
        double routersDelay @unit(s) = default(0.1ms);
        int numUe_A = default(1);
        int numUe_B = default(1);
        int numUe_C = default(1);
        int numUe_D = default(1);
        int numBgCells = default(0);
        bool enableMecOperatorB = default(true);
        bool enableMecOperatorC = default(true);
        bool enableMecOperatorD = default(true);

        @display("i=block/network2;bgb=2232.1401,1331.62");//;bgi=background/pisa_large");

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
        router1: Router {
            @display("p=907.705,198.785");
        }
        router2: Router {
            @display("p=1159.18,198.785");
        }
        router3: Router {
            @display("p=910.10004,50.295002");
        }
        router4: Router {
            @display("p=1159.18,50.295002");
        }

        // --- MNO A --- //

        upf_A: Upf {
            @display("p=910.10004,383.2");
        }
        upf_B: Upf {
            @display("p=1159.18,383.2");
        }
        upf_C: Upf {
            @display("p=749.635,383.2");
        }        
        upf_D: Upf {
            @display("p=1310.0651,383.2");
        }        
        
        iUpf_A: Upf {
            @display("p=907.705,490.975");
        }
        iUpf_B: Upf {
            @display("p=1159.18,490.975");
        }        
        iUpf_C: Upf {
            @display("p=749.635,490.975");
        }        
        iUpf_D: Upf {
            @display("p=1310.0651,490.975");
        }

        
        gnb_A1: gNodeB {
            @display("p=274.365,454.54498;is=vl");
        }
        gnb_A2: gNodeB {
            @display("p=795.14,661.02;is=vl");
        }
        gnb_A3: gNodeB {
            @display("p=1245.4,661.02;is=vl");
        }
        gnb_A4: gNodeB {
            @display("p=1743.56,661.02;is=vl");
        }
        gnb_B1: gNodeB {
            @display("p=328.11502,723.29004;is=vl");
        }
        gnb_B2: gNodeB {
            @display("p=847.83,723.29004;is=vl");
        }
        gnb_B3: gNodeB {
            @display("p=1302.88,723.29004;is=vl");
        }
        gnb_B4: gNodeB {
            @display("p=1803.435,723.29004;is=vl");
        }        
        gnb_C1: gNodeB {
            @display("p=407.15,802.325;is=vl");
        }
        gnb_C2: gNodeB {
            @display("p=910.10004,787.955;is=vl");
        }
        gnb_C3: gNodeB {
            @display("p=1367.545,787.955;is=vl");
        }
        gnb_C4: gNodeB {
            @display("p=1880.0751,802.325;is=vl");
        }        
        gnb_D1: gNodeB {
            @display("p=488.58002,900.52;is=vl");
        }
        gnb_D2: gNodeB {
            @display("p=972.37,900.52;is=vl");
        }
        gnb_D3: gNodeB {
            @display("p=1484.9,900.52;is=vl");
        }
        gnb_D4: gNodeB {
            @display("p=1949.53,900.52;is=vl");
        }
        ue_A[numUe_A]: NRUe {
            @display("p=716.10504,900.52");
        }
        ue_B[numUe_B]: NRUe {
            @display("p=936.445,900.52");
        }
        ue_C[numUe_C]: NRUe {
            @display("p=1096.91,912.495");
        }
        ue_D[numUe_D]: NRUe {
            @display("p=1281.3251,912.495");
        }
                
        
        mecHost_A: MECHost {
            @display("p=558.03503,490.975");
        }
        mecOrchestrator_A: MecOrchestrator {
            @display("p=558.03503,198.785");
        }
        ualcmp_A: UALCMP {
            @display("p=773.585,198.785");
        }

        mecHost_B: MECHost if enableMecOperatorB {
            @display("p=1530.405,490.975");
        }
        mecOrchestrator_B: MecOrchestrator if enableMecOperatorB {
            @display("p=1530.405,198.785");
        }
        ualcmp_B: UALCMP if enableMecOperatorB {
            @display("p=1310.0651,198.785");
        }

        mecHost_C: MECHost if enableMecOperatorC {
            @display("p=558.03503,356.855");
        }
        mecOrchestrator_C: MecOrchestrator if enableMecOperatorC {
            @display("p=558.03503,50.295002");
        }
        ualcmp_C: UALCMP if enableMecOperatorC {
            @display("p=773.585,50.295002");
        }

        mecHost_D: MECHost if enableMecOperatorD {
            @display("p=1530.405,356.855");
        }
        mecOrchestrator_D: MecOrchestrator if enableMecOperatorD {
            @display("p=1530.405,50.295002");
        }
        ualcmp_D: UALCMP if enableMecOperatorD {
            @display("p=1310.0651,50.295002");
        }

        bgCell[numBgCells]: BackgroundCell {
            @display("p=138.635,629.97375;is=vl");
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

        upf_C.pppg++ <--> Eth10G {  length = 300m; } <--> iUpf_C.pppg++;
        iUpf_C.pppg++ <--> Eth10G {  length = 300m; } <--> mecHost_C.ppp++ if enableMecOperatorC;
        iUpf_C.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_C1.ppp;
        iUpf_C.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_C2.ppp;
        iUpf_C.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_C3.ppp;
        iUpf_C.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_C4.ppp;

        upf_D.pppg++ <--> Eth10G {  length = 300m; } <--> iUpf_D.pppg++;
        iUpf_D.pppg++ <--> Eth10G {  length = 300m; } <--> mecHost_D.ppp++ if enableMecOperatorD;
        iUpf_D.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_D1.ppp;
        iUpf_D.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_D2.ppp;
        iUpf_D.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_D3.ppp;
        iUpf_D.pppg++ <--> Eth10G {  length = 300m; } <--> gnb_D4.ppp;
        
        //# MEC-related connections
        router1.pppg++ <--> Eth10G {  length = 300m; } <--> upf_A.filterGate;
        ualcmp_A.ppp++ <--> Eth10G {  length = 300m; } <--> router1.pppg++;
        ualcmp_A.toMecOrchestrator --> mecOrchestrator_A.fromUALCMP;
        ualcmp_A.fromMecOrchestrator <-- mecOrchestrator_A.toUALCMP;

        router2.pppg++ <--> Eth10G {  length = 300m; } <--> upf_B.filterGate;
        ualcmp_B.ppp++ <--> Eth10G {  length = 300m; } <--> router2.pppg++ if enableMecOperatorB;
        ualcmp_B.toMecOrchestrator --> mecOrchestrator_B.fromUALCMP if enableMecOperatorB;
        ualcmp_B.fromMecOrchestrator <-- mecOrchestrator_B.toUALCMP if enableMecOperatorB;

        router3.pppg++ <--> Eth10G {  length = 300m; } <--> upf_C.filterGate;
        ualcmp_C.ppp++ <--> Eth10G {  length = 300m; } <--> router3.pppg++ if enableMecOperatorC;
        ualcmp_C.toMecOrchestrator --> mecOrchestrator_C.fromUALCMP if enableMecOperatorC;
        ualcmp_C.fromMecOrchestrator <-- mecOrchestrator_C.toUALCMP if enableMecOperatorC;

        router4.pppg++ <--> Eth10G {  length = 300m; } <--> upf_D.filterGate;
        ualcmp_D.ppp++ <--> Eth10G {  length = 300m; } <--> router4.pppg++ if enableMecOperatorD;
        ualcmp_D.toMecOrchestrator --> mecOrchestrator_D.fromUALCMP if enableMecOperatorD;
        ualcmp_D.fromMecOrchestrator <-- mecOrchestrator_D.toUALCMP if enableMecOperatorD;

        //# X2 connections
        gnb_A1.x2++ <--> Eth10G {  length = 500m; } <--> gnb_A2.x2++;
        gnb_A2.x2++ <--> Eth10G {  length = 500m; } <--> gnb_A3.x2++;
        gnb_A3.x2++ <--> Eth10G {  length = 500m; } <--> gnb_A4.x2++;

        gnb_B1.x2++ <--> Eth10G {  length = 500m; } <--> gnb_B2.x2++;
        gnb_B2.x2++ <--> Eth10G {  length = 500m; } <--> gnb_B3.x2++;
        gnb_B3.x2++ <--> Eth10G {  length = 500m; } <--> gnb_B4.x2++;

        gnb_C1.x2++ <--> Eth10G {  length = 500m; } <--> gnb_C2.x2++;
        gnb_C2.x2++ <--> Eth10G {  length = 500m; } <--> gnb_C3.x2++;
        gnb_C3.x2++ <--> Eth10G {  length = 500m; } <--> gnb_C4.x2++;

        gnb_D1.x2++ <--> Eth10G {  length = 500m; } <--> gnb_D2.x2++;
        gnb_D2.x2++ <--> Eth10G {  length = 500m; } <--> gnb_D3.x2++;
        gnb_D3.x2++ <--> Eth10G {  length = 500m; } <--> gnb_D4.x2++;



        //# Routers connection
        router1.pppg++ <--> Eth10G {  length = parent.linkLength*2e5; } <--> router2.pppg++;
        router1.pppg++ <--> Eth10G {  length = parent.linkLength*2e5; } <--> router3.pppg++;
        router1.pppg++ <--> Eth10G {  length = parent.linkLength*2e5; } <--> router4.pppg++;

        router2.pppg++ <--> Eth10G {  length = parent.linkLength*2e5; } <--> router3.pppg++;
        router2.pppg++ <--> Eth10G {  length = parent.linkLength*2e5; } <--> router4.pppg++;

        router3.pppg++ <--> Eth10G {  length = parent.linkLength*2e5; } <--> router4.pppg++;

}
