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

package simu5g.apps.mec.DeviceApp;

import inet.applications.contract.IApp;

//
// This module models the Device App entity as defined by the ETSI Multi-access Edge Computing (MEC)
// specifications. The Device App can be instantiated on a User Equipment (UE) that wants to
// dynamically instantiate a MEC application within a MEC system. It establishes a TCP connection
// with the User App Lifecycle Management Proxy (UALCMP) entity (see the ~UALCMP module) and, upon
// receiving a MEC application instantiation request from another UE application, it sends a message
// to the UALCMP including the URI of the MEC application to be instantiated. Once the confirmation
// of successful instantiation is received, the Device App sends an acknowledgment to the UE
// application that requested it. Moreover,
// this module is responsible for requesting the deletion of the MEC application from the MEC system
// upon explicit request from the UE application.
//
simple DeviceApp like IApp
{
    string UALCMPAddress = default(""); // UALCMP connect address to connect to, may be left empty ("")
    int UALCMPPort = default(-1); // UALCMP port number to connect to
    int ualcmpLocalPort = default(8740); // UALCMP local port number

    string appProvider = default("");
    string appPackageSource = default("");

    string localAddress = default(""); // used local address for UE and UALCMP, may be left empty ("")
    int localPort = default(-1); // UE port number to listen on
    int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
    int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
    int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
    @display("i=block/app");

    gates:
        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
}
