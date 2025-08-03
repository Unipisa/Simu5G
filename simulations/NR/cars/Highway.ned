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
package simu5g.simulations.NR.cars;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.node.ethernet.Eth10G;
import simu5g.world.radio.LteChannelControl;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.common.binder.Binder;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.cars.NRCar;
import simu5g.rdoexperiment.GnbMetricsCollector;
import org.car2x.veins.subprojects.veins_inet.VeinsInetManager;

// 🛠️ RDO Experiment network with attack scenarios
network RDOExperiment
{
    parameters:
        double playgroundSizeX @unit(m); // x size of simulation area
        double playgroundSizeY @unit(m);
        double playgroundSizeZ @unit(m);
        @display("bgb=800,500;bgg=100,1,grey95");

        int numCar = default(4);  // Number of cars
        bool useAttackSchedule = default(false);  // Phase 2
        string attackScheduleFile = default("attack_schedule.csv");

        bool useRDO = default(false);  // Phase 3
        string rdoConfigFile = default("rdo_config.csv");

    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=50,75;is=s");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=50,141;is=s");
            config = xmldoc("demo.xml");  // Your network config file
        }

        veinsManager: VeinsInetManager {
            @display("p=50,263;is=s");
        }

        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }

        binder: Binder {
            @display("p=50,198;is=s");
        }

        carrierAggregation: CarrierAggregation {
            @display("p=50,320;is=s");
        }

        server: StandardHost {
            @display("p=660,136;is=n;i=device/server");
        }

        router: Router {
            @display("p=561,135;i=device/smallrouter");
        }

        upf: Upf {
            @display("p=462,136;is=n");
        }

        gnb0: gNodeB {
            @display("p=250,125;is=vl");
        }

        gnb1: gNodeB {
            @display("p=250,270;is=vl");
        }

        gnb2: gNodeB {
            @display("p=408,270;is=vl");
        }
        
        car[numCar]: NRCar {
    		@display("p=287,354,r,120;is=s");
		}
        
//        car[0]: NRCar {
//            @display("p=287,354,r,120;is=s");
//        }
        
        gnbMetricsCollector: GnbMetricsCollector {
        @display("p=50,350");
    	}

        // Optionally you can add GnbMetricsCollector if you want:
        // GnbMetricsCollector: GnbMetricsCollector {
        //     @display("p=50,263;is=s");
        // }

    connections allowunconnected:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gnb0.ppp;
        upf.pppg++ <--> Eth10G <--> gnb1.ppp;
        upf.pppg++ <--> Eth10G <--> gnb2.ppp;

        gnb0.x2++ <--> Eth10G <--> gnb1.x2++;
        gnb0.x2++ <--> Eth10G <--> gnb2.x2++;
        gnb1.x2++ <--> Eth10G <--> gnb2.x2++;
}





