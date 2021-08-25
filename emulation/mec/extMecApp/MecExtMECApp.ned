//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.emulation.mec.extMecApp;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import simu5g.world.radio.LteChannelControl;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.mec.MECHost;

import inet.node.inet.Router;

import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;

import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.LCMProxy.LCMProxy;



network MecExtMECApp
{
    parameters:

        int numCars = default(0);

        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=805.92,499.56");

    submodules:
//  		visualizer: IntegratedCanvasVisualizer {
//            @display("p=72.54,425.19598");
//        }
        routingRecorder: RoutingTableRecorder {
            @display("p=690,104.88;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=556.1875,34.3825");
        }
        channelControl: LteChannelControl {
            @display("p=690,24.84;is=s");
        }
        binder: Binder {
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

        //# MEC modules
        mecHost1: MECHost {
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


        router.pppg++ <--> Eth10G <--> upf.filterGate; 

	//#	mec
        mecHost1.ppp++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> lcmProxy.ppp++;

        lcmProxy.toMecOrchestrator --> mecOrchestrator.fromLcmProxy;
        lcmProxy.fromMecOrchestrator <-- mecOrchestrator.toLcmProxy;

        

}
