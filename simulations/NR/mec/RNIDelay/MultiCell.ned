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

package simu5g.simulations.NR.mec.RNIDelay;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.ExtCell;
import simu5g.nodes.Upf;
import simu5g.world.radio.LteChannelControl;
import simu5g.nodes.mec.MECHost;

network MultiCell
{
    parameters:
        int numExtCells = default(0);
        int numCars = default(0);
        @display("i=block/network2;bgb=991,558;bgi=background/budapest");
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
            @display("p=50.993748,258.7;is=s");
        }
        server: StandardHost {
            @display("p=148.00626,59.7;is=n;i=device/server");
        }
        upf: Upf {
            @display("p=517.4,74.625;is=l");
        }
        router: Router {
            @display("p=284.81876,58.45625;i=device/smallrouter");
        }
        gNodeB1: gNodeB {
            @display("p=264.91876,279.84375;is=vl");
        }
        gNodeB2: gNodeB {
            @display("p=560.9313,268.65;is=vl");
        }
        gNodeB3: gNodeB {
            @display("p=885.55,268.65;is=vl");
        }
        ue11: NRUe {
            @display("p=353.225,383.075");
        }
        ue12: NRUe {
            @display("p=477,363");
        }
        ue21: NRUe {
            @display("p=789,280");
        }
        ue22: NRUe {
            @display("p=915,183");
        }
        ue[numCars]: NRUe {
            @display("p=264.91876,405.4625");
        }

        extCell[numExtCells]: ExtCell {
            @display("p=100,480;is=vl");
        }
        mecHost: MECHost {
            @display("p=787.29376,79.6;is=l");
        }
    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gNodeB1.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB2.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB3.ppp;

        //# X2 connections
        gNodeB1.x2++ <--> Eth10G <--> gNodeB2.x2++;
        gNodeB2.x2++ <--> Eth10G <--> gNodeB3.x2++;


        //#	mec
        //# direct connection between gNodeBs and mecHost
        mecHost.ppp++ <--> Eth10G <--> gNodeB1.pppMEHost;
        mecHost.ppp++ <--> Eth10G <--> gNodeB2.pppMEHost;
        mecHost.ppp++ <--> Eth10G <--> gNodeB3.pppMEHost;

}
