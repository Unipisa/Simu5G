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

package simu5g.simulations.NR.mec.highway;


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
package simu5g.simulations.NR.networks;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.backgroundCell.BackgroundCell;
import simu5g.world.radio.LteChannelControl;
import simu5g.nodes.mec.MECHost;

import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;

// SingleCell_standalone
//
// This network represents the deployment option 3 in 3GPP 38.801
//
network rtp_network_highway
{
    parameters:
        int numUe = default(1);
        int numBgCells = default(0);
        @display("i=block/network2;bgb=2000,2000");//;bgi=background/pisa");
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
        binder: Binder {
            @display("p=50,175;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50.993748,258.7;is=s");
        }
        //        router: Router {
        //            @display("p=841.68,1591.1759;i=device/smallrouter");
        //        }
        upf: Upf {
            @display("p=869.73596,1719.432;is=l");
        }

        iUpf: Upf {
            @display("p=869.73596,1270.536");
        }

        gnb1: gNodeB {
            @display("p=632.812,599.092;is=vl");
        }

        gnb3: gNodeB {
            @display("p=490.064,662.036;is=vl");
        }

        gnb2: gNodeB {
            @display("p=780.05597,592.34796;is=vl");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=777.552,148.29599;is=vl");
        }
        ue[numUe]: NRUe {
            @display("i=device/car;is=vs");
            @display("p=694.632,676.648");
        }
        //# MEC module
        mecHost: MECHost {
            @display("p=1110.216,1270.536;is=l");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=1539.0719,1719.432;is=l");
        }

        ualcmp: UALCMP {
            @display("p=1234.464,1715.424;i=device/smallrouter");
        }

    connections:

//        router.pppg++ <--> Eth10G <--> upf.filterGate;

        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;

        iUpf.pppg++ <--> Eth10G <--> gnb1.ppp;
        iUpf.pppg++ <--> Eth10G <--> gnb2.ppp;
        iUpf.pppg++ <--> Eth10G <--> gnb3.ppp;


		// gnb x2 connections
        gnb1.x2++ <--> Eth10G <--> gnb2.x2++;
        gnb1.x2++ <--> Eth10G <--> gnb3.x2++;
        gnb2.x2++ <--> Eth10G <--> gnb3.x2++;


        mecHost.pppENB++ <--> Eth10G <--> iUpf.pppg++;

        ualcmp.ppp++ <--> Eth10G <--> upf.filterGate;

        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;


}
