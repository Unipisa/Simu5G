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

package simu5g.emulation.pimrc_extension;

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
import simu5g.nodes.PgwStandard;
import simu5g.nodes.ExtCell;
import simu5g.world.radio.LteChannelControl;


// ExtClientServerExample
//
// One real UE receives data from a real server
// 
network ExtClientServerExample
{
    parameters:
        int numBkUEs = default(0);
        int numExtCells = default(0);
        @display("i=block/network2;bgb=796,554.7125;bgi=background/pisa");
    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=73.38125,130.59375;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=73.38125,72.1375");
        }
        channelControl: LteChannelControl {
            @display("p=70.89375,189.05;is=s");
        }
        binder: Binder {
            @display("p=73.38125,261.1875;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=72.54,343.728;is=s");
        }
        server: StandardHost {
            @display("p=223.2,71.423996;is=n;i=device/server");
        }
        router: Router {
            @display("p=223.875,175.36874;i=device/smallrouter");
        }
        natRouter: Router {
            @display("p=345.96,175.21199;i=device/smallrouter");
        }
        upf: Upf {
            @display("p=460.1875,174.125;is=l");
        }
        gnb: gNodeB {
            @display("p=671.625,174.125;is=vl");
        }
        ue: NRUe {
            @display("p=620.496,309.132");
        }
        bkUe[numBkUEs]: NRUe {
            @display("p=728.748,309.132");
        }
        extCell[numExtCells]: ExtCell {
            @display("p=671.625,432.82498;is=vl");
        }
    connections allowunconnected:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> natRouter.pppg++;
        natRouter.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gnb.ppp;
}
