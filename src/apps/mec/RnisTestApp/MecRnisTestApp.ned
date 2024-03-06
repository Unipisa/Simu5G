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

package simu5g.apps.mec.RnisTestApp;

import inet.applications.contract.IApp;
import simu5g.nodes.mec.MECPlatform.IMECApp;

//
// This is a simple MEC app that connects to the Radio Network Information 
// Service and periodically requests L2 Measurements from it. 
// The response obtained from the RNIS is just sent back to the UE app. 
//
simple MecRnisTestApp like IMECApp, IApp
{
    parameters:
        @display("i=block/app;is=s");

        string interfaceTableModule = default("");

        int mecAppIndex;
        int mecAppId;
        int localUePort;

        string mp1Address = default("");
        int mp1Port = default(10021);

		// Resource required info
        double requiredRam @unit("B") = default(10MB);
        double requiredDisk @unit("B") = default(1MB);
        double requiredCpu = default(100);

        // IApp parameters
        int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        bool logger = default(false);

    gates:
        input mePlatformIn;
        output mePlatformOut;

        input socketIn;
        output socketOut;

}

