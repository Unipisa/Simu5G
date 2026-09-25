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

package simu5g.emulation.extclientserver_bgTraffic;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.corenetwork.bearerConfigurator.BearerConfigurator;
import simu5g.nodes.NrUe;
import simu5g.nodes.gNodeB;
import simu5g.nodes.Upf;
import simu5g.nodes.BackgroundCell;
import simu5g.stack.phy.medium.CellularRadioMedium;


//
// ~ExtClientServerExample with background cells instead of external ones: one
// ~gNodeB served by a ~Upf core, a router in front of the ~Upf, one ~NrUe, and
// numBgCells ~BackgroundCell cells. The router has no simulated peer: the host
// it talks to, and the applications on the ~NrUe, are configured in the ini
// file.
//
network ExtClientServer_bgCells
{
    parameters:
        // The gNBs connect to a 5GC (Upf), so every cellular stack has SDAP:
        // TS 37.324 ties SDAP to the 5GC connection, not to the radio
        **.cellularNic.hasSdap = default(true);
        @display("i=block/network2;bgb=800,554;bgi=background/pisa");

        // Number of BG cells
        int numBgCells = default(0);

    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=73,122;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=73,172;is=s");
        }
        binder: Binder {
            @display("p=73,222;is=s");
        }
        bearerConfigurator: BearerConfigurator {
            @display("p=73,272;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=73,322;is=s");
        }
        router: Router {
            @display("p=223,175;i=device/smallrouter");
        }
        upf: Upf {
            @display("p=460,174");
        }
        gnb: gNodeB {
            @display("p=671,174;is=vl");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=670,440;is=vl");
        }
        ue: NrUe {
            @display("p=670,309");
        }
        radioMedium: CellularRadioMedium {
            @display("p=73,147;is=s");
        }

    connections allowunconnected:
        router.pppg++ <--> Eth10G <--> upf.dnPppg;
        upf.pppg++ <--> Eth10G <--> gnb.ppp;
}
