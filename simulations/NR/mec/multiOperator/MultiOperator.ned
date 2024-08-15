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

package simu5g.simulations.NR.mec.multiOperator;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.Upf;
import simu5g.nodes.backgroundCell.BackgroundCell;
import simu5g.nodes.mec.MECHost;
import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;
import simu5g.world.radio.LteChannelControl;


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
        int numUe_A = default(1);
        int numUe_B = default(1);
        int numBgCells = default(0);
        bool enableMecOperatorB = default(true);
        @display("i=block/network2;bgb=1627,801;bgi=background/pisa");
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=50,75;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=50,125;is=s");
        }
        binder: Binder {
            @display("p=50,175;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50,258;is=s");
        }
        router: Router {
            @display("p=750,73");
        }
        upf_A: Upf {
            @display("p=625,173");
        }
        upf_B: Upf {
            @display("p=875,173");
        }
        iUpf_A: Upf {
            @display("p=623,281");
        }
        iUpf_B: Upf {
            @display("p=874,281");
        }
        gnb_A1: gNodeB {
            @display("p=274,454;is=vl");
        }
        gnb_A2: gNodeB {
            @display("p=800,452;is=vl");
        }
        gnb_A3: gNodeB {
            @display("p=1245,452;is=vl");
        }
        gnb_B1: gNodeB {
            @display("p=330,514;is=vl");
        }
        gnb_B2: gNodeB {
            @display("p=848,513;is=vl");
        }
        gnb_B3: gNodeB {
            @display("p=1304,513;is=vl");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=138,630;is=vl");
        }
        ue_A[numUe_A]: NRUe {
            @display("p=717,691");
        }
        ue_B[numUe_B]: NRUe {
            @display("p=937,691");
        }

        //# MEC modules
        mecHost_A: MECHost {
            @display("p=273,281");
        }
        mecHost_B: MECHost if enableMecOperatorB {
            @display("p=1245,281");
        }
        mecOrchestrator_A: MecOrchestrator {
            @display("p=273,73");
        }
        mecOrchestrator_B: MecOrchestrator if enableMecOperatorB {
            @display("p=1245,73");
        }
        ualcmp_A: UALCMP {
            @display("p=490,73");
        }
        ualcmp_B: UALCMP if enableMecOperatorB {
            @display("p=1027,73");
        }

    connections:

        //# 5G Core Network connections
        upf_A.pppg++ <--> Eth10G <--> iUpf_A.pppg++;
        iUpf_A.pppg++ <--> Eth10G <--> mecHost_A.ppp++;
        iUpf_A.pppg++ <--> Eth10G <--> gnb_A1.ppp;
        iUpf_A.pppg++ <--> Eth10G <--> gnb_A2.ppp;
        iUpf_A.pppg++ <--> Eth10G <--> gnb_A3.ppp;

        upf_B.pppg++ <--> Eth10G <--> iUpf_B.pppg++;
        iUpf_B.pppg++ <--> Eth10G <--> mecHost_B.ppp++ if enableMecOperatorB;
        iUpf_B.pppg++ <--> Eth10G <--> gnb_B1.ppp;
        iUpf_B.pppg++ <--> Eth10G <--> gnb_B2.ppp;
        iUpf_B.pppg++ <--> Eth10G <--> gnb_B3.ppp;

        //# MEC-related connections
        router.pppg++ <--> Eth10G <--> upf_A.filterGate;
        ualcmp_A.ppp++ <--> Eth10G <--> router.pppg++;
        ualcmp_A.toMecOrchestrator --> mecOrchestrator_A.fromUALCMP;
        ualcmp_A.fromMecOrchestrator <-- mecOrchestrator_A.toUALCMP;

        router.pppg++ <--> Eth10G <--> upf_B.filterGate;
        ualcmp_B.ppp++ <--> Eth10G <--> router.pppg++ if enableMecOperatorB;
        ualcmp_B.toMecOrchestrator --> mecOrchestrator_B.fromUALCMP if enableMecOperatorB;
        ualcmp_B.fromMecOrchestrator <-- mecOrchestrator_B.toUALCMP if enableMecOperatorB;

        //# X2 connections
        gnb_A1.x2++ <--> Eth10G <--> gnb_A2.x2++;
        gnb_A2.x2++ <--> Eth10G <--> gnb_A3.x2++;
        gnb_B1.x2++ <--> Eth10G <--> gnb_B2.x2++;
        gnb_B2.x2++ <--> Eth10G <--> gnb_B3.x2++;
}
