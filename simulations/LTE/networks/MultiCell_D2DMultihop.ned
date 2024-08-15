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
package simu5g.simulations.LTE.networks;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.apps.d2dMultihop.eventGenerator.EventGenerator;
import simu5g.apps.d2dMultihop.statistics.MultihopD2DStatistics;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.PgwStandard;
import simu5g.nodes.Ue;
import simu5g.nodes.eNodeB;
import simu5g.world.radio.LteChannelControl;


network MultiCell_D2DMultihop
{
    parameters:
        int numUe1 = default(1);
        int numUe2 = default(1);
        int numUe3 = default(1);
        int numUe4 = default(1);
        int numUe5 = default(1);
        @display("i=block/network2;bgb=2000,500;");
    submodules:
        channelControl: LteChannelControl {
            @display("p=90,24;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=90,70;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=90,121");
        }
        binder: Binder {
            @display("p=90,171;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50,258;is=s");
        }
        server: StandardHost {
            @display("p=500,50;is=n;i=device/server");
        }
        router: Router {
            @display("p=750,50;i=device/smallrouter");
        }
        router1: Router {
            @display("p=200,194;i=device/smallrouter");
        }
        router2: Router {
            @display("p=600,194;i=device/smallrouter");
        }
        router3: Router {
            @display("p=1000,194;i=device/smallrouter");
        }
        router4: Router {
            @display("p=1400,194;i=device/smallrouter");
        }
        router5: Router {
            @display("p=1800,194;i=device/smallrouter");
        }
        pgw: PgwStandard {
            @display("p=1000,50;is=l");
        }
        eNodeB1: eNodeB {
            @display("p=200,400;is=vl");
        }
        eNodeB2: eNodeB {
            @display("p=600,400;is=vl");
        }
        eNodeB3: eNodeB {
            @display("p=1000,400;is=vl");
        }
        eNodeB4: eNodeB {
            @display("p=1400,400;is=vl");
        }
        eNodeB5: eNodeB {
            @display("p=1800,400;is=vl");
        }
        ue1[numUe1]: Ue {
            @display("p=300,450;is=s");
        }
        ue2[numUe2]: Ue {
            @display("p=700,450;is=s");
        }
        ue3[numUe3]: Ue {
            @display("p=1100,450;is=s");
        }
        ue4[numUe4]: Ue {
            @display("p=1500,450;is=s");
        }
        ue5[numUe5]: Ue {
            @display("p=1900,450;is=s");
        }
        eventGenerator: EventGenerator {
            @display("p=90,222;is=s");
        }
        d2dMultihopStatistics: MultihopD2DStatistics {
            @display("p=90,272;is=s");
        }
    connections:
        router1.pppg++ <--> Eth10G <--> eNodeB1.ppp;
        router2.pppg++ <--> Eth10G <--> eNodeB2.ppp;
        router3.pppg++ <--> Eth10G <--> eNodeB3.ppp;
        router4.pppg++ <--> Eth10G <--> eNodeB4.ppp;
        router5.pppg++ <--> Eth10G <--> eNodeB5.ppp;
        router1.pppg++ <--> Eth10G <--> pgw.pppg++;
        router2.pppg++ <--> Eth10G <--> pgw.pppg++;
        router3.pppg++ <--> Eth10G <--> pgw.pppg++;
        router4.pppg++ <--> Eth10G <--> pgw.pppg++;
        router5.pppg++ <--> Eth10G <--> pgw.pppg++;
        router.pppg++ <--> Eth10G <--> pgw.filterGate;
        server.pppg++ <--> Eth10G <--> router.pppg++;
}
