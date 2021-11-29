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

package simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase;

// This class implements the general structure of a Mec Service. It holds all the TCP connections with 
// the Mec Applications and manages its lifecycle. It manages Request-Replay and Subscribe-Notify schemes.
// Every request is inserted in the queue and executed in FIFO order. Also subscription event are queued 
// in a separate queue and have priority. The execution times are calculated with the 
// calculateRequestServiceTime method. During initialization it saves all the eNodeB connected to the Mec Host 
// in which the service is running. Each connection is managed by the SocketManager object that implements the
// TCPSocket::CallbackInterface It must be subclassed and only the methods relative to the requests management 
// (e.g handleGETrequest) have to be implemented.

moduleinterface IMECService 
{
    parameters:
        @display("i=block/app");
        int requestQueueSize;
       	int subscriptionQueueSize;
       	
       	string serviceName;
       	string serviceVersion;
       	string serviceSerialize;
       	
       	string transportId;
       	string transportName;
       	string transportType;
       	string transportProtocol;
    
    	string catId;
    	string catName;
    	string catUri;
    	string catVersion;	
    	
    	string scopeOfLocality;
    	bool consumedLocalOnly;
    	
    	bool loadGenerator;
    	double betaa;
    	int numBGApps;


		string localAddress; // local address; may be left empty ("")
        int localPort;     // localPort number to listen on
        string dataTransferMode @enum("bytecount","object","bytestream"); // bytestream for emulation
        
        double requestServiceTime;
        double subscriptionServiceTime @unit(s);
        string serverThreadClass;

		@signal[requestQueueSize];
        @statistic[requestQueueSizeStat](title="Request queue size"; source="requestQueueSize"; record=mean,vector);
        @signal[responseTime];
        @statistic[responseTimeStat](title="Response time of foreground requests"; source="responseTime"; record=mean,vector);
        
}


