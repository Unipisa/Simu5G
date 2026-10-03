//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.test.moduletest.lib;

import inet.networklayer.configurator.contract.IL3NetworkConfigurator;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.corenetwork.bearerConfigurator.BearerConfigurator;
import simu5g.corenetwork.coreControl.CoreControl;
import simu5g.nodes.LteUe;
import simu5g.nodes.NrUe;
import simu5g.nodes.PgwStandard;
import simu5g.nodes.Upf;
import simu5g.nodes.eNodeB;
import simu5g.nodes.gNodeB;
import simu5g.world.radio.LteChannelControl;

//
// A 5G core network for the tests of PDU sessions: one ~gNodeB, whose ~Upf anchors the
// sessions and faces the data network (a router and a server), and numUe ~NrUe
// terminals. The UPF's Ethernet N6 link (with hasEthernetBridge) is left unconnected.
//
network NrSessionTestNetwork
{
    parameters:
        int numUe = default(1);
        **.cellularNic.hasSdap = default(true);
        @display("bgb=700,500");
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=50,75;is=s");
        }
        ipv6Configurator: <default("")> like IL3NetworkConfigurator if typename != "" {
            @display("p=50,325;is=s");
        }
        binder: Binder {
            @display("p=50,125;is=s");
        }
        coreControl: CoreControl {
            @display("p=50,175;is=s");
        }
        bearerConfigurator: BearerConfigurator {
            @display("p=50,225;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50,275;is=s");
        }
        server: StandardHost {
            @display("p=200,75;i=device/server");
        }
        router: Router {
            @display("p=350,75");
        }
        upf: Upf {
            @display("p=500,75");
        }
        gnb: gNodeB {
            @display("p=500,250");
        }
        ue[numUe]: NrUe {
            @display("p=500,400");
        }
    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> upf.dnPppg;
        upf.pppg++ <--> Eth10G <--> gnb.ppp;
}

//
// The same with an EPC: one ~eNodeB, whose ~PgwStandard anchors the sessions, and numUe
// ~LteUe terminals
//
network EpcSessionTestNetwork
{
    parameters:
        int numUe = default(1);
        @display("bgb=700,500");
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=50,75;is=s");
        }
        binder: Binder {
            @display("p=50,125;is=s");
        }
        coreControl: CoreControl {
            @display("p=50,175;is=s");
        }
        bearerConfigurator: BearerConfigurator {
            @display("p=50,225;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50,275;is=s");
        }
        server: StandardHost {
            @display("p=200,75;i=device/server");
        }
        router: Router {
            @display("p=350,75");
        }
        pgw: PgwStandard {
            @display("p=500,75");
        }
        eNB: eNodeB {
            @display("p=500,250");
        }
        ue[numUe]: LteUe {
            @display("p=500,400");
        }
    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> pgw.dnPppg;
        pgw.pppg++ <--> Eth10G <--> eNB.ppp;
}
