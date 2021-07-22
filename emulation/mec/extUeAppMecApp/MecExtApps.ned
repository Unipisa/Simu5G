//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.emulation.mec.extUeAppMecApp;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import lte.world.radio.LteChannelControl;
import lte.nodes.binder.LteBinder;
import lte.common.carrierAggregation.CarrierAggregation;
import lte.nodes.Upf;
import lte.nodes.NR.gNodeB;
import lte.nodes.NR.NRUe;
import lte.nodes.mec.MEHost;

import inet.node.inet.Router;

import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;

import lte.nodes.mec.MECOrchestrator.MecOrchestrator;
import lte.nodes.mec.LCMProxy.LCMProxy;



network MecExtApps
{
    parameters:

        int numCars = default(0);

        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=805.92,499.56");

    submodules:
  		visualizer: IntegratedCanvasVisualizer {
            @display("p=72.54,425.19598");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=690,104.88;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=556.1875,34.3825");
        }
        channelControl: LteChannelControl {
            @display("p=690,24.84;is=s");
        }
        binder: LteBinder {
            @display("p=474.27625,34.3825;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=368.095,34.3825;is=s");
        }
        upf: Upf {
            @display("p=284.16125,259.89127;is=l");
        }
        gNodeB1: gNodeB {
            @display("p=137.53,259.89127;is=vl");
        }
        router: Router {
            @display("p=544.0525,259.89127");
        }

        natRouter: Router {
            @display("p=429.78125,258.88;i=device/smallrouter");
        }

        //# MEC modules
        meHost1: MEHost {
            @display("p=694.72876,259.89127;is=l");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=435.84875,184.0475;is=l");
        }

        lcmProxy: LCMProxy {
            @display("p=544.0525,183.03625;i=device/smallrouter");
        }


        car: NRUe {
            @display("p=51.57375,259.89127");
        }

    connections allowunconnected:

        upf.pppg++ <--> Eth10G <--> gNodeB1.ppp;

        natRouter.pppg++ <--> Eth10G <--> upf.filterGate;
        natRouter.pppg++ <--> Eth10G <--> router.pppg++; 

	//#	mec
        meHost1.ppp++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> lcmProxy.ppp++;

        lcmProxy.toMecOrchestrator --> mecOrchestrator.fromLcmProxy;
        lcmProxy.fromMecOrchestrator <-- mecOrchestrator.toLcmProxy;

        

}
