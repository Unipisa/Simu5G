//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package lte.simulations.NR.mec_rest.RNIDelay;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import lte.nodes.binder.LteBinder;
import lte.common.carrierAggregation.CarrierAggregation;
import lte.nodes.NR.NRUe;
import lte.nodes.NR.gNodeB;
import lte.nodes.ExtCell;
import lte.nodes.Upf;
import lte.world.radio.LteChannelControl;
import lte.nodes.mec.MEHost;

network MecNetwork2Cell
{
    parameters:
        int numExtCells = default(0);

        int numCars = default(0);

//        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
//        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
//        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        
        @display("bgb=805.92,499.56");
        @display("i=block/network2;bgb=991,558;bgi=background/budapest");
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=50,75;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=50,125");
        }
        binder: LteBinder {
            @display("p=50,175;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50.993748,258.7;is=s");
        }
        server: StandardHost {
            @display("p=159.2,74.625;is=n;i=device/server");
        }
        upf: Upf {
            @display("p=415.4125,74.625;is=l");
        }
        router: Router {
            @display("p=266.1625,73.38125;i=device/smallrouter");
        }
        gNodeB1: gNodeB {
            @display("p=266.1625,279.84375;is=vl");
        }
        gNodeB2: gNodeB {
            @display("p=588.29376,268.65;is=vl");
        }
        

        car[numCars]: NRUe {
            @display("p=264.91876,405.4625");
        }
        extCell[numExtCells]: ExtCell {
            @display("p=92.0375,383.075;is=vl");
        }
        //# MEC module
//        meHost: MEHost {
//            @display("p=415.4125,184.075;is=l");
//        }

    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gNodeB1.ppp;
        upf.pppg++ <--> Eth10G <--> gNodeB2.ppp;

        //# X2 connections
        gNodeB1.x2++ <--> Eth10G <--> gNodeB2.x2++;

//        //#	mec
//        // direct connection between gNodeB1 and meHost
//        meHost.pppENB++ <--> Eth10G <--> gNodeB1.pppMEHost;
//        meHost.pppENB++ <--> Eth10G <--> gNodeB2.pppMEHost;

}
