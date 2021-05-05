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

package lte.nodes.mec.MECOrchestrator;

import inet.applications.contract.IApp;

simple MecOrchestratorManager  like IApp{

    parameters:
        @display("i=block/source");
        
        int localPort = default(4000);			// ALL UEApps have to send UDP Packets to this Port!
        int destPort = default(4000);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
        string mecHostList; // it contains a comma separated list of the the mec host of the mec system associated with this orchestrator
        string interfaceTableModule;
                
    gates:
        output socketOut;
        input socketIn;
}
