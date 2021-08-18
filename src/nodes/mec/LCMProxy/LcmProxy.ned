//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.nodes.mec.LCMProxy;

import inet.applications.contract.IApp;

// 
// This module implements (part of) the mx2 reference point a device app uses to 
// request lyfecycle operations of a MEC app (i.e. instantation, termination, relocation).
// This API follows the ETSI MEC specification of ETSI GS MEC 016 V2.2.1 (2020-04) and in 
// particular:
//   - GET /app_list Retrieve available application information.
//   - POST /app_contexts For requesting the creation of a new application context.
//   - DELETE /app_contexts/{contextId}  For requesting the deletion of an existing application context 
//
// Communications with the MEC orchestrator occur via OMNeT connections and messages


simple LcmProxy  like  IApp
{
    parameters:
        @display("i=block/source");
        @class("LcmProxy");
        string serviceName = default("LCMProxy");
        int requestQueueSize = default(0);
        int subscriptionQueueSize = default(0);
        string localAddress = default(""); // local address; may be left empty ("")
        int localPort = default(1000);     // localPort number to listen on
        string dataTransferMode @enum("bytecount","object","bytestream") = default("bytestream"); // bytestream for emulation
        
        double requestServiceTime @unit(s)  = default(5us);
        double subscriptionServiceTime @unit(s) = default(10us);
        
        string serverThreadClass = default("lte.nodes.mec.MECPlatform.MECServices.MECServiceBase.SocketManager");
        
        int destPort = default(4000);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
                        
    gates:
        output socketOut;
        input socketIn;
        
        output toMecOrchestrator;
        input  fromMecOrchestrator;
}
