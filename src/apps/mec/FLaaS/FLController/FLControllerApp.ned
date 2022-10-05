//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package simu5g.apps.mec.FLaaS.FLController;

import inet.applications.contract.IApp;
import simu5g.nodes.mec.MECPlatform.IMECApp;
//import simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase.IMECService;

// TODO describe it

simple FLControllerApp like IMECApp, IApp
{
    parameters:
        @display("i=block/app;is=s");
  			
  		// IMECApp parameters
        int packetSize = default(10);
        string interfaceTableModule = default("");
        
        int mecAppIndex;
        int mecAppId;
        int localUePort;
        
        string mp1Address = default("");
        int mp1Port = default(10021);
        
		 //resource required info
        double requiredRam @unit("B");
        double requiredDisk @unit("B");
        double requiredCpu;
        
        double instantiationTime @unit("s") = 1s;
        
        // IMECService parameters
        int requestQueueSize = default(0);
        int subscriptionQueueSize = default(0);
        string localAddress = default("^.virtualisationInfrastructure"); // local address; may be left empty ("")
        string dataTransferMode @enum("bytecount","object","bytestream") = default("bytestream"); // bytestream for emulation
        
        bool loadGenerator = default(false);
    	double betaa  = default(0);  // used only if loadGenerator is true
    	int numBGApps = default(0); // used only if loadGenerator is true
        
        double requestServiceTime  @unit(s) = default(1ms);
        double subscriptionServiceTime @unit(s) = default(1ms);
        
        string serverThreadClass = default("simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase.SocketManager");
                
        
        @display("i=block/app");
        @signal[requestQueueSize];
        @statistic[requestQueueSizeStat](title="Request queue size"; source="requestQueueSize"; record=mean,vector);
        @signal[responseTime];
        @statistic[responseTimeStat](title="Response time of foreground requests"; source="responseTime"; record=mean,vector); 
       





		// IApp parameters
		int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
		
    gates:  
	  input socketIn;
	  output socketOut;               
}
