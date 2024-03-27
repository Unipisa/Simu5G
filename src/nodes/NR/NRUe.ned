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


package simu5g.nodes.NR;

import simu5g.nodes.Ue;
import simu5g.stack.NRNicUe;
import org.car2x.veins.base.modules.*;

import simu5g.corenetwork.statsCollector.NRueStatsCollector;
// 
// User Equipment Module
//
module NRUe extends Ue
{
    parameters:
        nicType = default("NRNicUe");

        int nrMasterId @mutable = default(0);
        int nrMacNodeId @mutable = default(0); // TODO: this is not a real parameter
        int nrMacCellId @mutable = default(0); // TODO: this is not a real parameter

    gates:
        input nrRadioIn @directIn;     // connection to master    

    submodules:
        //# UeStatsCollector - for MEC
//        NRueCollector: NRueStatsCollector if hasRNISupport {
//            @display("p=73.687996,445.75198;is=s");
//        }

    connections allowunconnected:

        cellularNic.nrRadioIn <-- nrRadioIn;
}


module NRCarTrackable extends NRUe
{
    parameters:

        @display("i=device/car");
        string veinsmobilityType = default("org.car2x.veins.modules.mobility.traci.TraCIMobility"); //type of the mobility module

        //# Mobility
        mobility.typename = default("VeinsInetMobility");
        veinsmobility.typename = default("VeinsInetMobility");
        //# Network Layer specs
        *.routingTableModule = default(absPath(".ipv4.routingTable"));

        ipv4.configurator.typename = "HostAutoConfigurator";
        ipv4.configurator.interfaces = "cellular lo0";



    submodules:

        veinsmobility: <veinsmobilityType> like org.car2x.veins.base.modules.IMobility {
            parameters:
                @display("p=137.90399,71.604;i=block/cogwheel");
        }

}
