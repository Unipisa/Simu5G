// 
//                           Simu5G
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 
package lte.simulations.NR.networks;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import lte.nodes.binder.LteBinder;
import lte.common.carrierAggregation.CarrierAggregation;
import lte.nodes.eNodeB;
import lte.nodes.NR.gNodeB;
import lte.nodes.NR.NRUe;
import lte.world.radio.LteChannelControl;
import lte.epc.PgwStandardSimplified;
import lte.nodes.ExtCell;
import lte.nodes.mec.MECHost;

// SingleCell_withSecondaryGnb
//
// This network represents the deployment option 3a in 3GPP 38.801
// A gNodeB is serving as secondary node for a LTE eNodeB, which is connected
// to the LTE EPC. All data-plane traffic goes through the LTE eNodeB 
//
network SingleCell_withSecondaryGnb_mec
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
        }
        binder: LteBinder {
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
        pgw: PgwStandardSimplified {
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

        //# MEC module
        mecHost: MECHost {
            @display("p=626.85,151.7375;is=l");
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
        
         //#	mec
        // direct connection between base stations and mecHost
        mecHost.pppENB++ <--> Eth10G <--> masterEnb.pppMEHost;
        mecHost.pppENB++ <--> Eth10G <--> secondaryGnb.pppMEHost;
        
        
}
