// @author Alessandro Noferi

package lte.nodes.mec.MEPlatform.MeServices.LocationService;


import lte.nodes.mec.MEPlatform.MeServices.MeServiceBase.MeServiceBase;

import inet.applications.contract.IApp;


//
// Radio Network Information Service
// TCPApp that implements an HTTP server that responds to HTTP requests (e.g GET POST)
// from MeApplication, or any application that uses TCP connections.
// It stores all the connections in a map and gets the information for the responds by
// calling methods of EnodeB. (cross-call methods)
// 
// @see ~GenericAppMsg, ~TCPBasicClientApp, ~TelnetApp
//
simple LocationService like MeServiceBase, IApp
{
    parameters:
        int requestQueueSize = default(0);
        int subscriptionQueueSize = default(0);
        string localAddress = default(""); // local address; may be left empty ("")
        int localPort = default(1000);     // localPort number to listen on
        string dataTransferMode @enum("bytecount","object","bytestream") = default("bytestream"); // bytestream for emulation
        double LocationSubscriptionPeriod @unit(s)= default(1s);
        
        double requestServiceTime  = default(5);
        double subscriptionServiceTime @unit(s) = default(0.5us);
        
        string serverThreadClass = default("lte.nodes.mec.MEPlatform.MeServices.MeServiceBase.SocketManager");
        
        
		string serviceName = default("LocationService");
       	string serviceVersion = default("v2");
       	string serviceSerialize = default("JSON");
       	
       	string transportId = default("LocAPI");
       	string transportName = default("LocAPI");
       	string transportType = default("REST_HTTP");
       	string transportProtocol = default("HTTP");
    
    	string catId = default("");
    	string catName = default("Location");
    	string catUri = default("");
    	string catVersion = default("v2");	
        
        
        @display("i=block/app");
        @signal[requestQueueSize](type = long);
        @statistic[requestQueueSizeStat](title="Request queue size"; source="requestQueueSize"; record=mean,vector);

    gates:
        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
}

