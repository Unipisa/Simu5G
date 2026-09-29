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

package simu5g.simulations.nr.ethernet;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth1G;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.corenetwork.bearerConfigurator.BearerConfigurator;
import simu5g.corenetwork.coreControl.CoreControl;
import simu5g.nodes.NrUe;
import simu5g.nodes.Upf;
import simu5g.nodes.gNodeB;
import simu5g.world.radio.LteChannelControl;

//
// A 5G standalone network whose UPF anchors Ethernet PDU sessions: its Ethernet N6 link
// goes to dnStation, a host of the data network's Ethernet LAN, and its IP N6 link to a
// router and a server, for the UEs of IP sessions. With hasSecondGnb, a second gNB
// (with an X2 link to the first) lets a UE hand over. device[] are the Ethernet devices
// behind ue[0], which is then an Ethernet bridge (see the ethernetMode parameter of
// ~LteUe).
//
network EthernetSessions
{
    parameters:
        **.cellularNic.hasSdap = default(true);
        int numUe = default(1);
        int numDevices = default(0);
        bool hasSecondGnb = default(false);
        upf.hasEthernetBridge = default(true);
        @display("i=block/network2;bgb=1000,700");
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=50,125;is=s");
        }
        binder: Binder {
            @display("p=50,175;is=s");
        }
        coreControl: CoreControl {
            @display("p=50,375;is=s");
        }
        bearerConfigurator: BearerConfigurator {
            @display("p=50,225;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50,275;is=s");
        }
        server: StandardHost {
            @display("p=212,118;is=n;i=device/server");
        }
        router: Router {
            @display("p=351,115;i=device/smallrouter");
        }
        dnStation: StandardHost {
            @display("p=351,215;i=device/pc");
        }
        upf: Upf {
            @display("p=500,115");
        }
        gnb1: gNodeB {
            @display("p=250,350;is=vl");
        }
        gnb2: gNodeB if hasSecondGnb {
            @display("p=750,350;is=vl");
        }
        ue[numUe]: NrUe {
            @display("p=500,500");
        }
        device[numDevices]: StandardHost {
            @display("p=500,650,row,100;i=device/device");
        }
    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> upf.dnPppg;
        upf.dnEthg <--> Eth1G <--> dnStation.ethg++;
        upf.pppg++ <--> Eth10G <--> gnb1.ppp;
        upf.pppg++ <--> Eth10G <--> gnb2.ppp if hasSecondGnb;
        gnb1.x2++ <--> Eth10G <--> gnb2.x2++ if hasSecondGnb;
        for i=0..numDevices-1 {
            device[i].ethg++ <--> Eth1G <--> ue[0].ethg++;
        }
}
