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

package lte.nodes.mec.MEPlatform.ServiceRegistry;

import inet.applications.contract.IApp;
import lte.nodes.mec.MEPlatform.MeServices.MeServiceBase.MeServiceBase;


// This module takes trace of all the MeServices in the MePlatform.
// MeServices register themself with:
// 	- ServiceName -> IpAddress and port
// MeApps look for the desired MeService by its name and if present the
// corresponding couple IpAddress:port.

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
        
        double requestServiceTime  = default(5);
        double subscriptionServiceTime @unit(s) = default(0.5us);
        
        string serverThreadClass = default("lte.nodes.mec.MEPlatform.MeServices.MeServiceBase.SocketManager");
        
        int destPort = default(4000);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
                
    gates:
        output socketOut;
        input socketIn;
}