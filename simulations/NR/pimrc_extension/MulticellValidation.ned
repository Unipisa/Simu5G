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

package simu5g.simulations.NR.pimrc_extension;

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
import simu5g.nodes.ExtCell;
import simu5g.world.radio.LteChannelControl;


// MulticellValidation
//
network MulticellValidation
{
    parameters:
        int numBkUEs = default(0);
        int numExtBkUEs0 = default(0);
        int numExtBkUEs1 = default(0);
        int numExtBkUEs2 = default(0);
        int numExtBkUEs3 = default(0);
        int numExtBkUEs4 = default(0);
        int numExtBkUEs5 = default(0);
        @display("i=block/network2;bgb=1208.6279,751.068;bgi=background/pisa");
    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=66.96,501.08398;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=66.96,441.936");
        }
        channelControl: LteChannelControl {
            @display("p=64.728,559.11597;is=s");
        }
        binder: Binder {
            @display("p=66.96,631.656;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=66.96,696.384;is=s");
        }
        server: StandardHost {
            @display("p=95.976,174.096;is=n;i=device/server");
        }
        extServer0: StandardHost {
            @display("p=135.036,70.308;is=n;i=device/server");
        }
        extServer1: StandardHost {
            @display("p=223.2,70.308;is=n;i=device/server");
        }
        extServer2: StandardHost {
            @display("p=308.016,70.308;is=n;i=device/server");
        }
        extServer3: StandardHost {
            @display("p=135.036,264.492;is=n;i=device/server");
        }
        extServer4: StandardHost {
            @display("p=223.2,264.492;is=n;i=device/server");
        }
        extServer5: StandardHost {
            @display("p=308.016,264.492;is=n;i=device/server");
        }
        serverUl: StandardHost {
            @display("p=95.976,174.096;is=n;i=device/server");
        }
        extServerUl0: StandardHost {
            @display("p=135.036,70.308;is=n;i=device/server");
        }
        extServerUl1: StandardHost {
            @display("p=223.2,70.308;is=n;i=device/server");
        }
        extServerUl2: StandardHost {
            @display("p=308.016,70.308;is=n;i=device/server");
        }
        extServerUl3: StandardHost {
            @display("p=135.036,264.492;is=n;i=device/server");
        }
        extServerUl4: StandardHost {
            @display("p=223.2,264.492;is=n;i=device/server");
        }
        extServerUl5: StandardHost {
            @display("p=308.016,264.492;is=n;i=device/server");
        }
        router: Router {
            @display("p=223.875,175.36874;i=device/smallrouter");
        }
        natRouter: Router {
            @display("p=345.96,175.21199;i=device/smallrouter");
        }
        upf: Upf {
            @display("p=460.1875,174.125;is=l");
        }
        gnb: gNodeB {
            @display("p=862.66797,386.136;is=vl");
        }
        extGnb0: gNodeB {
            @display("p=687.456,174.096;is=vl");
        }
        extGnb1: gNodeB {
            @display("p=1116,386.136;is=vl");
        }
        extGnb2: gNodeB {
            @display("p=697.5,578.088;is=vl");
        }
        extGnb3: gNodeB {
            @display("p=989.89197,174.096;is=vl");
        }
        extGnb4: gNodeB {
            @display("p=989.89197,578.088;is=vl");
        }
        extGnb5: gNodeB {
            @display("p=562.464,386.136;is=vl");
        }
        ue: NRUe {
            @display("p=920.69995,419.616");
        }
        bkUe[numBkUEs]: NRUe {
            @display("p=831.42,501.08398");
        }
        extBkUe0[numExtBkUEs0]: NRUe {
            @display("p=687.456,78.119995");
        }
        extBkUe1[numExtBkUEs1]: NRUe {
            @display("p=1116,501.08398");
        }
        extBkUe2[numExtBkUEs2]: NRUe {
            @display("p=697.5,685.224");
        }
        extBkUe3[numExtBkUEs3]: NRUe {
            @display("p=989.89197,78.119995");
        }
        extBkUe4[numExtBkUEs4]: NRUe {
            @display("p=989.89197,685.224");
        }
        extBkUe5[numExtBkUEs5]: NRUe {
            @display("p=562.464,492.15598");
        }
    connections allowunconnected:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        extServer0.pppg++ <--> Eth10G <--> router.pppg++;
        extServer1.pppg++ <--> Eth10G <--> router.pppg++;
        extServer2.pppg++ <--> Eth10G <--> router.pppg++;
        extServer3.pppg++ <--> Eth10G <--> router.pppg++;
        extServer4.pppg++ <--> Eth10G <--> router.pppg++;
        extServer5.pppg++ <--> Eth10G <--> router.pppg++;
        serverUl.pppg++ <--> Eth10G <--> router.pppg++;
        extServerUl0.pppg++ <--> Eth10G <--> router.pppg++;
        extServerUl1.pppg++ <--> Eth10G <--> router.pppg++;
        extServerUl2.pppg++ <--> Eth10G <--> router.pppg++;
        extServerUl3.pppg++ <--> Eth10G <--> router.pppg++;
        extServerUl4.pppg++ <--> Eth10G <--> router.pppg++;
        extServerUl5.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> natRouter.pppg++;
        natRouter.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gnb.ppp;
        upf.pppg++ <--> Eth10G <--> extGnb0.ppp;
        upf.pppg++ <--> Eth10G <--> extGnb1.ppp;
        upf.pppg++ <--> Eth10G <--> extGnb2.ppp;
        upf.pppg++ <--> Eth10G <--> extGnb3.ppp;
        upf.pppg++ <--> Eth10G <--> extGnb4.ppp;
        upf.pppg++ <--> Eth10G <--> extGnb5.ppp;
}
