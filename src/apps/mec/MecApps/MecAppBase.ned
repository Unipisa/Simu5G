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

package simu5g.apps.mec.MecApps;

import inet.applications.contract.IApp;
import simu5g.nodes.mec.MECPlatform.IMECApp;

//
//  This is a MEC app that ... TBC
//
simple MecAppBase like IMECApp, IApp
{
    parameters:
        @display("i=block/app;is=s");

        int mecAppId;
        int mecAppIndex;

        string mp1Address = default("");
        int mp1Port = default(10021);

        // modules:
        string vimModule = default("^.vim");
        string mecPlatformModule = default("^.mecPlatform");
        string serviceRegistryModule = default("^.mecPlatform.serviceRegistry");

        //resource required info
        double requiredRam @unit("B");
        double requiredDisk @unit("B");
        double requiredCpu;

        // socket parameters
        int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

    gates:
        input mePlatformIn;
        output mePlatformOut;

        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
}

