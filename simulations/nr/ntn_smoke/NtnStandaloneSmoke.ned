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

package simu5g.simulations.nr.ntn_smoke;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.mobility.georeference.GeographicReferenceSystem;
import simu5g.nodes.NtnGateway;
import simu5g.nodes.NrUe;
import simu5g.nodes.Satellite;
import simu5g.nodes.Upf;
import simu5g.nodes.gNodeB;
import simu5g.world.radio.LteChannelControl;

//
// Minimal standalone NTN smoke topology that keeps the existing core-network
// path intact and adds the NTN relay pair used by the Binder association.
//
network NtnStandaloneSmoke
{
    parameters:
        int numUe = default(1);
        @display("i=block/network2;bgb=1000,600;bgi=background/pisa");
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
        carrierAggregation: CarrierAggregation {
            @display("p=50,258;is=s");
        }

        geoRef: GeographicReferenceSystem {
            @display("p=50,345;is=s");
        }

        server: StandardHost {
            @display("p=180,120;is=n;i=device/server");
        }
        router: Router {
            @display("p=300,120;i=device/smallrouter");
        }
        upf: Upf {
            @display("p=430,120");
        }
        iUpf: Upf {
            @display("p=580,120");
        }
        gnb: gNodeB {
            @display("p=720,250");
        }
        ntnGateway: NtnGateway {
            @display("p=720,390");
        }
        satellite: Satellite {
            @display("p=900,390");
        }
        ue[numUe]: NrUe {
            @display("p=720,510");
        }
    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gnb.ppp;
        gnb.ntn <--> ntnGateway.gnbLink;
}
