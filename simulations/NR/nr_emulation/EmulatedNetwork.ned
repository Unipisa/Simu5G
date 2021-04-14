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
package simu5g.simulations.NR.nr_emulation;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.world.radio.LteChannelControl;
import simu5g.nodes.ExtCell;

network EmulatedNetwork
{
    parameters:
        int numExtCells = default(0);
        @display("i=block/network2;bgb=796,554.7125;bgi=background/pisa");
    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=73.38125,130.59375;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=73.38125,72.1375");
            config = xmldoc("demo.xml");
        }
        channelControl: LteChannelControl {
            @display("p=70.89375,189.05;is=s");
        }
        binder: Binder {
            @display("p=73.38125,261.1875;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50.993748,258.7;is=s");
        }
        router: Router {
            @display("p=223.875,175.36874;i=device/smallrouter");
        }
        upf: Upf {
            @display("p=460.1875,174.125;is=l");
        }
        gnb: gNodeB {
            @display("p=671.625,174.125;is=vl");
        }
        ue: NRUe {
            @display("p=671.625,309.69376");
        }
        extCell[numExtCells]: ExtCell {
            @display("p=671.625,432.82498;is=vl");
        }
    connections:
        router.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gnb.ppp;
}
