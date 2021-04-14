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
import simu5g.nodes.eNodeB;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.PgwStandard;
import simu5g.nodes.ExtCell;
import simu5g.world.radio.LteChannelControl;

// SingleCell_withSecondaryGnb
//
// This network represents the deployment option 3a in 3GPP 38.801
// A gNodeB is serving as secondary node for a LTE eNodeB, which is connected
// to the LTE EPC. All data-plane traffic goes through the LTE eNodeB 
//
network SingleCell_withSecondaryGnb
{
    parameters:
        int numUe = default(1);
        int numExtCells = default(0);
        @display("i=block/network2;bgb=991,558;bgi=background/pisa");
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=50,75;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=50,125");
            config = xmldoc("demo.xml");
        }
        binder: Binder {
            @display("p=50,175;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50.993748,258.7;is=s");
        }
        server: StandardHost {
            @display("p=210.19374,63.431248;is=n;i=device/server");
        }
        router: Router {
            @display("p=361.93124,62.1875;i=device/smallrouter");
        }
        pgw: PgwStandard {
            @display("p=524.8625,63.431248;is=l");
        }
        masterEnb: eNodeB {
            @display("p=527.35,277.35626;is=vl");
        }
        secondaryGnb: gNodeB {
            @display("p=726.35,277.35626;is=vl");
        }
        ue[numUe]: NRUe {
            @display("p=628.09375,411.68124");
        }
        extCell[numExtCells]: ExtCell {
            @display("p=100,480;is=vl");
        }
    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> pgw.filterGate;
        pgw.pppg++ <--> Eth10G <--> masterEnb.ppp;

        //# X2 connections
        masterEnb.x2++ <--> Eth10G <--> secondaryGnb.x2++;
}
