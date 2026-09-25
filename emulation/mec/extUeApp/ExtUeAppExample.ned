//
//                           Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.emulation.mec.extUeApp;

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
import simu5g.stack.phy.medium.CellularRadioMedium;


//
// One ~gNodeB and one ~MecHost hung off the same intermediate ~Upf, behind an
// anchor ~Upf that carries the ~Ualcmp and, behind it, the ~MecOrchestrator, plus
// one ~NrUe. Nothing here is external: which node talks to a real host, and over
// which interface, is configured in the ini file.
//
// ~ExtUeAppExample, ~ExtMecAppExample and ~ExtUeAppMecAppExample share this
// topology and differ only in that configuration.
//
network ExtUeAppExample
{
    parameters:
        // The gNBs connect to a 5GC (Upf), so every cellular stack has SDAP:
        // TS 37.324 ties SDAP to the 5GC connection, not to the radio
        **.cellularNic.hasSdap = default(true);

        @display("bgb=805,500");

    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=690,74;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=690,124;is=s");
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
            @display("p=420,259");
        }
        iUpf: Upf {
            @display("p=277,259");
        }
        gNodeB1: gNodeB {
            @display("p=137,260;is=vl");
        }

        //# MEC modules
        mecHost1: MecHost {
            @display("p=277,158;is=l");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=520,158;is=l");
        }

        ualcmp: Ualcmp {
            @display("p=420,157;i=device/smallrouter");
        }

        ue: NrUe {
            @display("p=51,259");
        }
        radioMedium: CellularRadioMedium {
            @display("p=690,99;is=s");
        }

    connections allowunconnected:

        gNodeB1.ppp <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> upf.pppg++;

	    //#	mec
        mecHost1.ppp++ <--> Eth10G <--> iUpf.pppg++;
        upf.dnPppg <--> Eth10G <--> ualcmp.ppp++;

        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;
}
