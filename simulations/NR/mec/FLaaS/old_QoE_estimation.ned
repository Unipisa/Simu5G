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

package simu5g.simulations.NR.mec.FLaaS;


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

import inet.node.rtp.RtpHost;


// SingleCell_standalone
//
// This network represents the deployment option 3 in 3GPP 38.801
//
network old_QoE_estimation
{
    parameters:
        int numUes = default(1);
        int numBgCells = default(0);
        @display("i=block/network2;bgb=2000,2100");//;bgi=background/pisa");
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

        upf: Upf {
            @display("p=1523.296,597.536;is=l");
        }

        iUpf: Upf {
            @display("p=1241.36,589.12");
        }

        gnb1: gNodeB {
            @display("p=632.812,599.092;is=vl");
        }
        gnb2: gNodeB {
            @display("p=206.192,963.632;is=vl");
        }
        gnb3: gNodeB {
            @display("p=490.064,662.036;is=vl");
        }
        gnb4: gNodeB {
            @display("p=558.628,800.28796;is=vl");
        }
        gnb5: gNodeB {
            @display("p=741.83997,439.48398;is=vl");
        }
        gnb6: gNodeB {
            @display("p=773.312,773.312;is=vl");
        }
        gnb7: gNodeB {
            @display("p=780.05597,592.34796;is=vl");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=363.052,258.52;is=vl");
        }
        ue[numUes]: NRUe {
            @display("p=694.632,676.648");
        }

        //# MEC module
        mecHost: MECHost {
            @display("p=1241.36,172.528;is=l");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=1851.52,122.032;is=l");
        }

        ualcmp: UALCMP {
            @display("p=1523.296,117.824;i=device/smallrouter");
        }


    connections:

        iUpf.pppg++ <--> Eth10G <--> gnb1.ppp;
        iUpf.pppg++ <--> Eth10G <--> gnb2.ppp;
        iUpf.pppg++ <--> Eth10G <--> gnb3.ppp;
        iUpf.pppg++ <--> Eth10G <--> gnb4.ppp;
        iUpf.pppg++ <--> Eth10G <--> gnb5.ppp;
        iUpf.pppg++ <--> Eth10G <--> gnb6.ppp;
        iUpf.pppg++ <--> Eth10G <--> gnb7.ppp;

		// gnb1 x2 connections
        gnb1.x2++ <--> Eth10G <--> gnb2.x2++;
        gnb1.x2++ <--> Eth10G <--> gnb3.x2++;
        gnb1.x2++ <--> Eth10G <--> gnb4.x2++;
        gnb1.x2++ <--> Eth10G <--> gnb5.x2++;
        gnb1.x2++ <--> Eth10G <--> gnb6.x2++;
        gnb1.x2++ <--> Eth10G <--> gnb7.x2++;

		// gnb2 x2 connections
        gnb2.x2++ <--> Eth10G <--> gnb3.x2++;
        gnb2.x2++ <--> Eth10G <--> gnb4.x2++;
        gnb2.x2++ <--> Eth10G <--> gnb5.x2++;
        gnb2.x2++ <--> Eth10G <--> gnb6.x2++;
        gnb2.x2++ <--> Eth10G <--> gnb7.x2++;

		// gnb3 x2 connections
        gnb3.x2++ <--> Eth10G <--> gnb4.x2++;
        gnb3.x2++ <--> Eth10G <--> gnb5.x2++;
        gnb3.x2++ <--> Eth10G <--> gnb6.x2++;
        gnb3.x2++ <--> Eth10G <--> gnb7.x2++;

        // gnb4 x2 connections
        gnb4.x2++ <--> Eth10G <--> gnb5.x2++;
        gnb4.x2++ <--> Eth10G <--> gnb6.x2++;
        gnb4.x2++ <--> Eth10G <--> gnb7.x2++;

        // gnb5 x2 connections
        gnb5.x2++ <--> Eth10G <--> gnb6.x2++;
        gnb5.x2++ <--> Eth10G <--> gnb7.x2++;

        // gnb6 x2 connections
        gnb6.x2++ <--> Eth10G <--> gnb7.x2++;


        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        ualcmp.ppp++ <--> Eth10G <--> upf.filterGate;

        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;

        mecHost.ppp++ <--> Eth10G <--> iUpf.pppg++;
}
