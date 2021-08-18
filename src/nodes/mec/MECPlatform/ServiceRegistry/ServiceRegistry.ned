//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.nodes.mec.MECPlatform.ServiceRegistry;
import inet.applications.contract.IApp;

// This module aims to provide essential functionalities of the Service Registry entity of 
// the MEC architecture (ETSI GS MEC 003).
// Service discovery is available via REST api, according to the ETSI GS MEC 011
// Each Service Registry maintains the MEC service end point of each MEC service in the
// MEC system.
// Mec services register themself with:
// 	- ServiceName -> IpAddress and port

simple ServiceRegistry like IApp
{
        parameters:
        @display("i=block/source");
        @class("ServiceRegistry");
        
        string serviceName = default("ServiceRegistry");
        int requestQueueSize = default(0);
        int subscriptionQueueSize = default(0);
        string localAddress = default(""); // local address; may be left empty ("")
        int localPort = default(1000);     // localPort number to listen on
        string dataTransferMode @enum("bytecount","object","bytestream") = default("bytestream"); // bytestream for emulation
        
        double requestServiceTime  @unit(s) = default(15us);
        double subscriptionServiceTime @unit(s) = default(20us);
        
        string serverThreadClass = default("lte.nodes.mec.MECPlatform.MECServices.MECServiceBase.SocketManager");
        
        int destPort = default(4000);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
                
    gates:
        output socketOut;
        input socketIn;
}