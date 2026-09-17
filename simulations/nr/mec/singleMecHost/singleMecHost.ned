//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.simulations.nr.mec.singleMecHost;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.corenetwork.bearerConfigurator.BearerConfigurator;
import simu5g.nodes.NrUe;
import simu5g.nodes.gNodeB;
import simu5g.nodes.Upf;
import simu5g.nodes.MecHost;
import simu5g.mec.orchestrator.MecOrchestrator;
import simu5g.mec.ualcmp.Ualcmp;
import simu5g.world.radio.LteChannelControl;


//
// One ~gNodeB and one ~MecHost hung off the same intermediate ~Upf, behind an
// anchor ~Upf that carries the ~Ualcmp and, behind it, the ~MecOrchestrator.
// numUes ~NrUe terminals attach to the gNodeB.
//
network SingleMecHost
{
    parameters:
        // The gNBs connect to a 5GC (Upf), so every cellular stack has SDAP:
        // TS 37.324 ties SDAP to the 5GC connection, not to the radio
        **.cellularNic.hasSdap = default(true);
        @display("i=block/network2;bgb=811,507;bgi=background/pisa");

        int numUes = default(0);

        double playgroundSizeX @unit(m); // X size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // Y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // Z size of the area the nodes are in (in meters)

    submodules:

        routingRecorder: RoutingTableRecorder {
            @display("p=690,74;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=690,124;is=s");
        }
        channelControl: LteChannelControl {
            @display("p=690,24;is=s");
        }
        binder: Binder {
            @display("p=690,174;is=s");
        }
        bearerConfigurator: BearerConfigurator {
            @display("p=690,224;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=690,274;is=s");
        }
        upf: Upf {
            @display("p=545,250");
        }
        iUpf: Upf {
            @display("p=352,250");
        }
        gNodeB1: gNodeB {
            @display("p=140,250;is=vl");
        }
        ue[numUes]: NrUe {
            @display("p=237,310");
        }
        //# MEC module
        mecHost: MecHost {
            @display("p=352,145;is=l");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=544,63;is=l");
        }

        ualcmp: Ualcmp {
            @display("p=545,158;i=device/smallrouter");
        }


    connections allowunconnected:

        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gNodeB1.ppp;
        ualcmp.ppp++ <--> Eth10G <--> upf.dnPppg;

        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;

        mecHost.ppp++ <--> Eth10G <--> iUpf.pppg++;
}
