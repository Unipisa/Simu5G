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

package lte.apps.mec.MeApps;

import inet.applications.contract.IApp;
import lte.nodes.mec.MEPlatform.IMEApp;


simple MecAppBase like IApp, IMEApp
{
    parameters:
        @class(MecAppBase);
        
  		// IMEApp parameters
        int mecAppId;
        
        int localUePort;
        
        string mp1Address = default("meHost.mecPlatform");
        int mp1Port = default(10021);
        
		 //resource required info
        double requiredRam @unit("B") = default(10MB);
        double requiredDisk @unit("B") = default(100MB);
        double requiredCpu = default(15000);

		// IApp parameters
		int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value


    gates:
	  input mePlatformIn;
	  output mePlatformOut;
	  
	  input socketIn;
	  output socketOut;
	  			    
}