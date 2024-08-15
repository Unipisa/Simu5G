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
package simu5g.simulations.NR.networks;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.PgwStandard;
import simu5g.nodes.backgroundCell.BackgroundCell;
import simu5g.nodes.eNodeB;
import simu5g.world.radio.LteChannelControl;


// MultiCell_withSecondaryGnb
//
// This network represents the deployment option 3a in 3GPP 38.801
// A gNodeB is serving as secondary node for a LTE eNodeB, which is connected
// to the LTE EPC. All data-plane traffic goes through the LTE eNodeB
//
network MultiCell_withSecondaryGnb
{
    parameters:
        int numUe = default(1);
        int numBgCells = default(0);
        @display("i=block/network2;bgb=1100,800;bgi=background/pisa");
    submodules:
        channelControl: LteChannelControl {
            @display("p=104,62;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=106,131;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=50,172");
        }
        binder: Binder {
            @display("p=106,334;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=106,418;is=s");
        }
        server: StandardHost {
            @display("p=273,48;is=n;i=device/server");
        }
        router: Router {
            @display("p=393,46;i=device/smallrouter");
        }
        pgw: PgwStandard {
            @display("p=530,49;is=n");
        }
        masterEnb1: eNodeB {
            @display("p=363,277;is=vl");
        }
        secondaryGnb1: gNodeB {
            @display("p=527,461;is=vl");
        }
        masterEnb2: eNodeB {
            @display("p=692,277;is=vl");
        }
        secondaryGnb2: gNodeB {
            @display("p=856,461;is=vl");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=1000,107;is=vl");
        }
        ue[numUe]: NRUe {
            @display("p=526,334");
        }
    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> pgw.filterGate;
        pgw.pppg++ <--> Eth10G <--> masterEnb1.ppp;
        pgw.pppg++ <--> Eth10G <--> masterEnb2.ppp;

        //# X2 connections
        secondaryGnb1.x2++ <--> Eth10G <--> masterEnb1.x2++;
        masterEnb1.x2++ <--> Eth10G <--> masterEnb2.x2++;
        masterEnb2.x2++ <--> Eth10G <--> secondaryGnb2.x2++;
}
