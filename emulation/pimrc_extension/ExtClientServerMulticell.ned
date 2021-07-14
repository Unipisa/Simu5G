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

package simu5g.emulation.pimrc_extension;

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
import simu5g.nodes.PgwStandard;
import simu5g.nodes.ExtCell;
import simu5g.world.radio.LteChannelControl;


// ExtClientServerMulticell
//
// One real UE receives data from a real server
// 
network ExtClientServerMulticell
{
    parameters:
        int numBgCells = default(1);
        int numBkUEs = default(0);
        int numExtBkUEs0 = default(0);
        int numExtBkUEs1 = default(0);
        int numExtBkUEs2 = default(0);
        int numExtBkUEs3 = default(0);
        int numExtBkUEs4 = default(0);
        int numExtBkUEs5 = default(0);
        int numExtBkUEs6 = default(0);
        int numExtBkUEs7 = default(0);
        int numExtBkUEs8 = default(0);
        int numExtCells = default(0);
        @display("i=block/network2;bgb=796,554.7125;bgi=background/pisa");
    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=73.38125,130.59375;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=73.38125,72.1375");
        }
        channelControl: LteChannelControl {
            @display("p=70.89375,189.05;is=s");
        }
        binder: Binder {
            @display("p=73.38125,261.1875;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=72.54,343.728;is=s");
        }
        server: StandardHost {
            @display("p=223.2,71.423996;is=n;i=device/server");
        }
        extServer0: StandardHost if numBgCells>=1 {
            @display("p=135.036,70.308;is=n;i=device/server");
        }
        extServer1: StandardHost if numBgCells>=2 {
            @display("p=223.2,70.308;is=n;i=device/server");
        }
        extServer2: StandardHost if numBgCells>=3 {
            @display("p=308.016,70.308;is=n;i=device/server");
        }
        extServer3: StandardHost if numBgCells>=4 {
            @display("p=135.036,264.492;is=n;i=device/server");
        }
        extServer4: StandardHost if numBgCells>=5 {
            @display("p=223.2,264.492;is=n;i=device/server");
        }
        extServer5: StandardHost if numBgCells>=6 {
            @display("p=308.016,264.492;is=n;i=device/server");
        }
        extServer6: StandardHost if numBgCells>=7 {
            @display("p=135.036,264.492;is=n;i=device/server");
        }
        extServer7: StandardHost if numBgCells>=8 {
            @display("p=223.2,264.492;is=n;i=device/server");
        }
        extServer8: StandardHost if numBgCells>=9 {
            @display("p=308.016,264.492;is=n;i=device/server");
        }
        serverUl: StandardHost {
            @display("p=95.976,174.096;is=n;i=device/server");
        }
        extServerUl0: StandardHost if numBgCells>=1 {
            @display("p=135.036,70.308;is=n;i=device/server");
        }
        extServerUl1: StandardHost if numBgCells>=2 {
            @display("p=223.2,70.308;is=n;i=device/server");
        }
        extServerUl2: StandardHost if numBgCells>=3 {
            @display("p=308.016,70.308;is=n;i=device/server");
        }
        extServerUl3: StandardHost if numBgCells>=4 {
            @display("p=135.036,264.492;is=n;i=device/server");
        }
        extServerUl4: StandardHost if numBgCells>=5 {
            @display("p=223.2,264.492;is=n;i=device/server");
        }
        extServerUl5: StandardHost if numBgCells>=6 {
            @display("p=308.016,264.492;is=n;i=device/server");
        }
        extServerUl6: StandardHost if numBgCells>=7 {
            @display("p=135.036,264.492;is=n;i=device/server");
        }
        extServerUl7: StandardHost if numBgCells>=8 {
            @display("p=223.2,264.492;is=n;i=device/server");
        }
        extServerUl8: StandardHost if numBgCells>=9 {
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
            @display("p=671.625,174.125;is=vl");
        }
        bgCell[numBgCells]: gNodeB {
            @display("p=670.716,440.81998;is=vl");
        }
        ue: NRUe {
            @display("p=620.496,309.132");
        }
        bkUe[numBkUEs]: NRUe {
            @display("p=728.748,309.132");
        }
        extBkUe0[numExtBkUEs0]: NRUe if numBgCells>=1 {
            @display("p=687.456,78.119995");
        }
        extBkUe1[numExtBkUEs1]: NRUe if numBgCells>=2 {
            @display("p=1116,501.08398");
        }
        extBkUe2[numExtBkUEs2]: NRUe if numBgCells>=3 {
            @display("p=697.5,685.224");
        }
        extBkUe3[numExtBkUEs3]: NRUe if numBgCells>=4 {
            @display("p=989.89197,78.119995");
        }
        extBkUe4[numExtBkUEs4]: NRUe if numBgCells>=5 {
            @display("p=989.89197,685.224");
        }
        extBkUe5[numExtBkUEs5]: NRUe if numBgCells>=6 {
            @display("p=562.464,492.15598");
        }
        extBkUe6[numExtBkUEs6]: NRUe if numBgCells>=7 {
            @display("p=989.89197,78.119995");
        }
        extBkUe7[numExtBkUEs7]: NRUe if numBgCells>=8 {
            @display("p=989.89197,685.224");
        }
        extBkUe8[numExtBkUEs8]: NRUe if numBgCells>=9 {
            @display("p=562.464,492.15598");
        }
        extCell[numExtCells]: ExtCell {
            @display("p=495.504,440.81998;is=vl");
        }
    connections allowunconnected:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        extServer0.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=1;
        extServer1.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=2;
        extServer2.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=3;
        extServer3.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=4;
        extServer4.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=5;
        extServer5.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=6;
        extServer6.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=7;
        extServer7.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=8;
        extServer8.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=9;
        serverUl.pppg++ <--> Eth10G <--> router.pppg++;
        extServerUl0.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=1;
        extServerUl1.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=2;
        extServerUl2.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=3;
        extServerUl3.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=4;
        extServerUl4.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=5;
        extServerUl5.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=6;
        extServerUl6.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=7;
        extServerUl7.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=8;
        extServerUl8.pppg++ <--> Eth10G <--> router.pppg++ if numBgCells>=9;
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> natRouter.pppg++;
        natRouter.pppg++ <--> Eth10G <--> upf.filterGate;
        upf.pppg++ <--> Eth10G <--> gnb.ppp;

        for i=0..numBgCells-1 {
            upf.pppg++ <--> Eth10G <--> bgCell[i].ppp;
        }
}
