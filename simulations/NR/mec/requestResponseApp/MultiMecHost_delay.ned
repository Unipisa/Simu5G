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
package simu5g.simulations.NR.mec.requestResponseApp;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
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


// MultiMecHost
//
// This network includes two gNBs and two MEC hosts
//
network MultiMecHost_delay
{
    parameters:
        int numUe = default(1);
        int numBgCells = default(0);
        double routersDelay @unit(m) = default(0m);
//        volatile double routersDelay @unit(m) = default(normal(2m, 0.8m));

        @display("i=block/network2;bgb=1088,678;");//bgi=background/pisa");
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=50,75;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=50,125");
        }
        binder: Binder {
            @display("p=50,175;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50,258;is=s");
        }
        upf: Upf {
            @display("p=495,132");
        }
        iUpf1: Upf {
            @display("p=400,247");
        }
        iUpf2: Upf {
            @display("p=590,247");
        }
        gnb1: gNodeB {
            @display("p=283,421;is=vl");
        }
        gnb2: gNodeB {
            @display("p=749,400;is=vl");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=80,576;is=vl");
        }
        ue[numUe]: NRUe {
            @display("p=353,481");
        }

        //# MEC modules
        mecHost1: MECHost {
            @display("p=248,247");
        }
        mecHost2: MECHost {
            @display("p=748,247");
        }
        mecOrchestrator: MecOrchestrator {
            @display("p=749,132");
        }
        ualcmp: UALCMP {
            @display("p=603,131");
        }

    connections:

        //# 5G Core Network connections
        upf.pppg++ <--> Eth10G <--> iUpf1.pppg++;
        upf.pppg++ <--> Eth10G <--> iUpf2.pppg++;
        iUpf1.pppg++ <--> Eth10G <--> gnb1.ppp;
        iUpf2.pppg++ <--> Eth10G <--> gnb2.ppp;
        iUpf2.pppg++ <--> Eth10G {  length = parent.routersDelay*2e5; } <--> iUpf1.pppg++;


        //# MEC-related connections
        ualcmp.ppp++ <--> Eth10G <--> upf.filterGate;
        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;
        mecHost1.ppp++ <--> Eth10G <--> iUpf1.pppg++;
        mecHost2.ppp++ <--> Eth10G <--> iUpf2.pppg++;



}
