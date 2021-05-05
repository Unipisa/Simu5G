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

package simu5g.simulations.NR.pimrc_extension;

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
import simu5g.nodes.backgroundCell.BackgroundCell;
import simu5g.world.radio.LteChannelControl;


// MulticellBackgroundValidation
//
network MulticellBackgroundValidation
{
    parameters:
        int numBkUEs = default(0);
        int numBgCell = default(0);
        @display("i=block/network2;bgb=1208.6279,751.068;bgi=background/pisa");
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
            @display("p=222.705,71.205;is=n;i=device/server");
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
            @display("p=692.355,168.165;is=vl");
        }
        bgCell[numBgCell]: BackgroundCell {
            @display("p=692.355,380.26498;is=vl");
        }
        ue: NRUe {
            @display("p=851.43,187.86");
        }
    connections allowunconnected:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> natRouter.pppg++;
        natRouter.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gnb.ppp;
}
