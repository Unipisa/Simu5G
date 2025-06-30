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


package simu5g.nodes.cars;

import inet.applications.contract.IApp;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.networklayer.configurator.ipv4.HostAutoConfigurator;
import inet.transportlayer.tcp.Tcp;
import inet.transportlayer.udp.Udp;
import inet.common.MessageDispatcher;
import simu5g.nodes.Ue;
import simu5g.nodes.NR.NRUe;

import org.car2x.veins.base.modules.*;

//
// This module defines a Car node as a LTE User Equipment (UE) with vehicular-like
// mobility. This behavior is modeled by utilizing the Veins-Inet project.
//
// See https://veins.car2x.org/documentation/modules/#veins_inet
//
module Car extends Ue
{
    parameters:
        @display("i=device/car");

        //# Mobility
        mobility.typename = default("VeinsInetMobility");

        //# Network Layer specs
        *.routingTableModule = default(absPath(".ipv4.routingTable"));
        *.interfaceTableModule = default(absPath(".interfaceTable"));

        ipv4.configurator.typename = "HostAutoConfigurator";
        ipv4.configurator.interfaces = "cellular lo0";
}


