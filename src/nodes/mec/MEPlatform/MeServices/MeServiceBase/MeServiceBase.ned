
//


// @author Alessandro Noferi
// This class implements the general structure of a Mec Service.
// It holds all the TCP connections with the e.g Mec Applications and
// manages its lifecycle.
// It manages Request-Replay and Subscribe-Notify schemes.
// Every request is inserted in the queue and executed in FIFO order.
// Also subscription event are queued in a separate queue and have the priority.
// The execution times are calculated with the calculateRequestServiceTime method.
// During initialization it saves all the eNodeB connected to the MeHost in
// which the service is running.
// Each connection is managed by the SocketManager object that implements the
// TCPSocket::CallbackInterface
// It must be subclassed and only the methods relative to the requests management (e.g handleGETrequest)
// have to be implemented.


package lte.nodes.mec.MEPlatform.MeServices.MeServiceBase;


moduleinterface MeServiceBase 
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
       	
//   	servDescriptor.name = par("serviceName").str();
//    servDescriptor.category = par("serviceCategory").str();
//    servDescriptor.version = par("serviceVersion").str();
//    servDescriptor.serialize = par("serviceSerialize").str();
//    servDescriptor.transportId = par("transportId").str();
//    servDescriptor.transportName = par("transportName").str();
//    servDescriptor.transportType = par("transportType").str();
//    servDescriptor.transportProtocol = par("transportProtocol").str();
//    
//    servDescriptor.catId = par("catId").str();
//    servDescriptor.catName = par("catName").str();
//    servDescriptor.catHref = par("catUri").str();
//    servDescriptor.catVersion = par("catVersion").str();

		string localAddress; // local address; may be left empty ("")
        int localPort;     // localPort number to listen on
        string dataTransferMode @enum("bytecount","object","bytestream"); // bytestream for emulation
        
        double requestServiceTime;
        double subscriptionServiceTime @unit(s);
        string serverThreadClass;

		@signal[requestQueueSize](type = long);
        @statistic[requestQueueSizeStat](title="Request queue size"; source="requestQueueSize"; record=mean,vector);

//    gates:
//        input socketIn @labels(TcpCommand/up);
//        output socketOut @labels(TcpCommand/down);
}


