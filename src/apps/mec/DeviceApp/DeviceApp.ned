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

simple DeviceApp like IApp
{
    string UALCMPAddress = default(""); // may be left empty ("")
    int UALCMPPort = default(-1); // port number to listen on
    
    string appProvider = default("");
    string appPackageSource =  default("");
    
    string localAddress = default(""); // may be left empty ("")
    int localPort = default(-1); // port number to listen on
    string connectAddress = default("");  // server address (may be symbolic)
    int connectPort = default(1000); // port number to connect to
    double startTime @unit(s) = default(1s); // time first session begins
    double stopTime @unit(s) = default(-1s);  // time of finishing sending, negative values mean forever
    int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
    int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
    int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
    @display("i=block/app");
    @lifecycleSupport;
    double stopOperationExtraTime @unit(s) = default(-1s);    // extra time after lifecycle stop operation finished
    double stopOperationTimeout @unit(s) = default(2s);    // timeout value for lifecycle stop operation

    gates:
        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
    
}