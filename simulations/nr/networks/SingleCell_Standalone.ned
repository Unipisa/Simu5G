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
import simu5g.world.radio.LteChannelControl;


//
// One ~gNodeB served by a 5G core: the anchor ~Upf faces the data network (a
// router and a server), and an intermediate ~Upf sits between it and the
// gNodeB. numUe ~NrUe terminals attach to the gNodeB, and numBgCells
// ~BackgroundCell cells can be added around it as interferers.
//
// The topology is 5G standalone, deployment option 2 of 3GPP TR 38.801.
//
network SingleCell_Standalone
{
    parameters:
        int numUe = default(1);
        int numBgCells = default(0);
        // The gNB connects to a 5GC (Upf), so every cellular stack has SDAP:
        // TS 37.324 ties SDAP to the 5GC connection, not to the radio
        **.cellularNic.hasSdap = default(true);
        @display("i=block/network2;bgb=1000,1000;bgi=background/pisa");
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
        bearerConfigurator: BearerConfigurator {
            @display("p=50,225;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50,275;is=s");
        }
        server: StandardHost {
            @display("p=212,118;is=n;i=device/server");
        }
        router: Router {
            @display("p=363,115;i=device/smallrouter");
        }
        upf: Upf {
            @display("p=527,116");
        }
        iUpf: Upf {
            @display("p=725,118");
        }
        gnb: gNodeB {
            @display("p=726,277;is=vl");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=416,277;is=vl");
        }
        ue[numUe]: NrUe {
            @display("p=628,411");
        }
    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> upf.dnPppg;
        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gnb.ppp;
}
