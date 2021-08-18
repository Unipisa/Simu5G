//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.nodes.mec.MECPlatform.MECServices.RNIService;


import lte.nodes.mec.MECPlatform.MECServices.MECServiceBase.IMECService;
import inet.applications.contract.IApp;

//# RNI service module


simple RNIService like IMECService, IApp
{
    parameters:
        int requestQueueSize = default(0);
        int subscriptionQueueSize = default(0);
        string localAddress = default(""); // local address; may be left empty ("")
        int localPort = default(1000);     // localPort number to listen on
        string dataTransferMode @enum("bytecount","object","bytestream") = default("bytestream"); // bytestream for emulation
        
        double requestServiceTime @unit(s) = default(0.5us);
        double subscriptionServiceTime @unit(s) = default(0.5us);
        
        bool loadGenerator = default(false);
    	double betaa = default(0);  // used only if loadGenerator is true
    	int numBGApps = default(0); // used only if loadGenerator is true
    	
    	        
        string serverThreadClass = default("lte.nodes.mec.MEPlatform.MeServices.MeServiceBase.SocketManager");
        
		string serviceName = default("RNIService");
       	string serviceVersion = default("v2");
       	string serviceSerialize = default("JSON");
       	
       	string transportId = default("RNIAPI");
       	string transportName = default("RNIAPI");
       	string transportType = default("REST_HTTP");
       	string transportProtocol = default("HTTP");
    
    	string catId = default("");
    	string catName = default("RNI");
    	string catUri = default("");
    	string catVersion = default("v2");	
    	
    	string scopeOfLocality = default("MEC_HOST");
    	bool consumedLocalOnly = default(true);        
        
        @display("i=block/app");
        @signal[requestQueueSize](type = long);
        @statistic[requestQueueSizeStat](title="Request queue size"; source="requestQueueSize"; record=mean,vector);

    gates:
        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
}

