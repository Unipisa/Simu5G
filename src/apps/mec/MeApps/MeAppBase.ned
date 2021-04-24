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


moduleinterface MeAppBase 
{
    parameters:
        string localAddress; // may be left empty ("")
        int localPort; // port number to listen on
        string connectAddress;  // server address (may be symbolic)
        int connectPort ; // port number to connect to
        double startTime ; // time first session begins
        double stopTime ;  // time of finishing sending, negative values mean forever
        int timeToLive ; // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp ; // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos ; // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
        @display("i=block/app");
        @lifecycleSupport;
        double stopOperationExtraTime @unit(s);    // extra time after lifecycle stop operation finished
        double stopOperationTimeout @unit(s);    // timeout value for lifecycle stop operation

		 //resource required infos
        int requiredRam @unit("B");
        int requiredDisk @unit("B");
        double requiredCpu;

    gates:
	  input mePlatformIn;
	  output mePlatformOut;
			    


}