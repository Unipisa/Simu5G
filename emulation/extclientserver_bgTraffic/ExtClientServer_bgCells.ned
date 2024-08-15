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

package simu5g.emulation.extclientserver_bgTraffic;

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
import simu5g.world.radio.LteChannelControl;


// ExtClientServer_bgCells
//
// One real UE receives data from a real server
// The network includes a configurable number of background cells
//
network ExtClientServer_bgCells
{
    parameters:
        @display("i=block/network2;bgb=800,554;bgi=background/pisa");

        // number of BG cells
        int numBgCells = default(0);

    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=73,130;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=73,72");
        }
        channelControl: LteChannelControl {
            @display("p=70,190;is=s");
        }
        binder: Binder {
            @display("p=73,261;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=72,343;is=s");
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
        ue: NRUe {
            @display("p=670,309");
        }

    connections allowunconnected:
        router.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gnb.ppp;
}
