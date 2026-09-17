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
// ~SingleCell_Standalone with its UEs split into three vectors, so that a
// configuration can address cellular terminals (ueCell), D2D transmitters
// (ueD2DTx) and D2D receivers (ueD2DRx) separately. The rest of the topology is
// the same: one ~gNodeB behind an intermediate and an anchor ~Upf, with a router
// and a server on the data network, plus numBgCells ~BackgroundCell interferers.
//
network SingleCell_Standalone_D2D
{
    parameters:
        // The gNB connects to a 5GC (Upf), so every cellular stack has SDAP:
        // TS 37.324 ties SDAP to the 5GC connection, not to the radio
        **.cellularNic.hasSdap = default(true);
        int numUeCell = default(0);
        int numUeD2DTx = default(0);
        int numUeD2DRx = default(0);
        int numBgCells = default(0);
        @display("i=block/network2;bgb=826,558;bgi=background/pisa");

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
        ueCell[numUeCell]: NrUe {
            @display("p=612,316");
        }
        ueD2DTx[numUeD2DTx]: NrUe {
            @display("p=550,423");
        }
        ueD2DRx[numUeD2DRx]: NrUe {
            @display("p=726,423");
        }
    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> upf.dnPppg;
        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gnb.ppp;
}
