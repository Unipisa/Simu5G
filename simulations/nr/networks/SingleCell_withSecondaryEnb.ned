//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//
package simu5g.simulations.nr.networks;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.corenetwork.bearerConfigurator.BearerConfigurator;
import simu5g.nodes.NrUe;
import simu5g.nodes.gNodeB;
import simu5g.nodes.Upf;
import simu5g.nodes.BackgroundCell;
import simu5g.nodes.eNodeB;
import simu5g.world.radio.LteChannelControl;


//
// The mirror image of ~SingleCell_withSecondaryGnb: here the gNodeB is the master
// node and the eNodeB is its secondary, so the UE is anchored on its NR stack and
// its LTE stack carries the secondary cell group. All data-plane traffic enters at
// the gNodeB.
//
// Unlike the EN-DC network, the core is a 5GC (Upf): that is NE-DC's shape in
// TS 37.340 -- an NR master pairs with a 5GC -- so its bearers are "5gc" bearers,
// selected by QFI, and SDAP is in the stack. The secondary eNodeB contributes the
// secondary cell group's radio leg over X2 and has no core-network link of its
// own; its bearers' PDCP terminates at the master (MN-terminated).
//
network SingleCell_withSecondaryEnb
{
    parameters:
        int numUe = default(1);
        int numBgCells = default(0);
        // The gNB connects to a 5GC (Upf), so every cellular stack has SDAP:
        // TS 37.324 ties SDAP to the 5GC connection, not to the radio
        **.cellularNic.hasSdap = default(true);
        @display("i=block/network2;bgb=991,558;bgi=background/pisa");
    submodules:
        channelControl: LteChannelControl {
            @display("p=88,24;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=88,74;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=88,124;is=s");
        }
        binder: Binder {
            @display("p=88,174;is=s");
        }
        bearerConfigurator: BearerConfigurator {
            @display("p=88,224;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=88,274;is=s");
        }
        server: StandardHost {
            @display("p=210,63;is=n;i=device/server");
        }
        router: Router {
            @display("p=361,62;i=device/smallrouter");
        }
        upf: Upf {
            @display("p=524,63;is=l");
        }
        iUpf: Upf {
            @display("p=524,170;is=l");
        }
        masterGnb: gNodeB {
            @display("p=527,277;is=vl");
        }
        secondaryEnb: eNodeB {
            @display("p=726,277;is=vl");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=910,98;is=vl");
        }
        ue[numUe]: NrUe {
            @display("p=628,411");
        }
    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> upf.dnPppg;
        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> masterGnb.ppp;

        //# X2 connections
        masterGnb.x2++ <--> Eth10G <--> secondaryEnb.x2++;
}
