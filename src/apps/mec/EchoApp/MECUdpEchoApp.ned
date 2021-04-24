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

package lte.apps.mec.EchoApp;

import inet.applications.udpapp.UdpEchoApp;

import inet.applications.contract.IApp;

//
// Listens on an UDP port, and sends back each received packet to its sender.
// Note: when used together with ~UdpBasicApp, ~UdpBasicApp's "received packet
// lifetime" statistic will record round-trip times.
//
simple MECUdpEchoApp extends UdpEchoApp
{
    parameters:
        
        string ueSimbolicAddress; 		// i.e. UE[x] or car[x]
        int uePort;
        string localAddress;	// i.e meHost.virtualisationInfrastructure

		//resource required infos
        int requiredRam @unit("B") = default(10MB);
        int requiredDisk @unit("B") = default(100MB);
        double requiredCpu = default(0.01);
        

}