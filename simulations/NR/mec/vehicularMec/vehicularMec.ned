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

package simu5g.simulations.NR.mec.vehicularMec;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import simu5g.world.radio.LteChannelControl;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.NR.NRCarTrackable;
import simu5g.nodes.mec.MECHost;
import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;
import simu5g.nodes.cars.NRCar;
import org.car2x.veins.modules.world.annotations.AnnotationManager;
import org.car2x.veins.visualizer.roads.RoadsCanvasVisualizer;


import org.car2x.veins.subprojects.veins_inet.VeinsInetManager;


import org.car2x.veins.base.connectionManager.ConnectionManager;
import org.car2x.veins.base.modules.BaseWorldUtility;
import org.car2x.veins.modules.mobility.traci.TraCIScenarioManager*;
import org.car2x.veins.nodes.Car;

import org.car2x.veins.modules.mobility.traci.TraCIVehicleInserter;


network vehicularMec
{
    parameters:
        //@display("i=block/network2;bgb=7423.889,6058.327;bgi=background/evry");
        @display("i=block/network2;bgb=7423.889,6058.327");
        //@display("bgb=$playgroundSizeX,$playgroundSizeY");

        //int numUes = default(0);
        //int numCars = default(0);



        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)


        string moduleType = default("simu5g.nodes.NR.NRCarTrackable");  // module type to be used in the simulation for each managed vehicle      
        string moduleName = default("car");  // module name to be used in the simulation for each managed vehicle


        //veinsManager.moduleType = default("simu5g.nodes.NR.NRCarTrackable");  // module type to be used in the simulation for each managed vehicle      
        //veinsManager.moduleName = default("car");  // module name to be used in the simulation for each managed vehicle


    submodules:
        TraCIVehicleInserter: TraCIVehicleInserter {
            @display("p=5928.636,2873.3882");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=6827.8105,1289.6975;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=6827.8105,3153.798;is=s");
        }
        channelControl: LteChannelControl {
            @display("p=6816.9727,704.45667;is=s");
        }
        binder: Binder {
            @display("p=6827.8105,227.59369;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=6827.8105,1929.1274;is=s");
        }
        manager: VeinsInetManager {
            @display("p=6827.8105,2503.5305;is=s");
        }

        iUpf: Upf {
            @display("p=1988.336,606.2;is=s");
        }

        annotations: AnnotationManager {
            @display("p=5928.636,363.72");
        }
        roadsCanvasVisualizer: RoadsCanvasVisualizer {
            @display("p=6818.692,3817.619");
        }


        upf: Upf {
            @display("p=1988.336,218.23201;is=s");
        }
        gNodeB1: gNodeB {
            @display("p=860.804,606.2;is=l");
        }
        car[0]: NRCarTrackable {
            @display("p=11723.908,6643.952");
        }

        //# MEC module
        mecHost: MECHost {
            @display("p=1988.336,1151.78;is=s");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=2376.3042,703.192");
        }

        ualcmp: UALCMP {
            @display("p=2376.3042,218.23201;i=device/smallrouter;is=s");
        }




        //TEST
        connectionManager: ConnectionManager {
            parameters:
                @display("p=5928.636,1151.78;i=abstract/multicast");
        }

        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeX", 1525, "parent.playgroundSizeX");
                playgroundSizeY = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeY", 1525, "parent.playgroundSizeY");
                playgroundSizeZ = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeZ", 1525, "parent.playgroundSizeZ");
                @display("p=5916.512,2012.5841;i=misc/globe");
        }


    connections allowunconnected:



        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gNodeB1.ppp;
        ualcmp.ppp++ <--> Eth10G <--> upf.filterGate;

        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;

        mecHost.ppp++ <--> Eth10G <--> iUpf.pppg++;
}
