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

package simu5g.apps.mec.FLaaS.FLLearner;

import inet.applications.contract.IApp;
import simu5g.nodes.mec.MECPlatform.IMECApp;


simple MecFLLearnerApp like IMECApp, IApp
{
    parameters:
        @display("i=block/app;is=s");
  			
        int packetSize = default(10);
        string interfaceTableModule = default("");
        
//        volatile double trainingDuration @unit("s") = uniform(6s, 9s);
		volatile double trainingDuration @unit("s") = default(uniform(50s, 60s, 3));
        volatile double localModelSize @unit("B") = uniform(70kB, 80kB, 3);
        string parentModule = default("MEC_HOST");
        int shortTraininagProbabilty = default(bernoulli(0.90));
        volatile double shortTrainingDuration @unit("s") = default(exponential(50s, 3));
        volatile double longTrainingDuration @unit("s") = default(exponential(80s, 3));
        
        int mecAppIndex;
        int mecAppId;
        int localUePort;
        
        string localAddress = default("");
        int localPort = default(9000);
        
        
        string mp1Address = default("");
        int mp1Port = default(10021);
        
		 //resource required info
        double requiredRam @unit("B");
        double requiredDisk @unit("B");
        double requiredCpu;

		// IApp parameters
		int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
        @signal[flaas_startRoundSignal];
        @statistic[flaas_startRoundSignal](title="startRoundSignal"; unit=""; source="flaas_startRoundSignal"; record=vector; interpolationmode=none;);
        @signal[flaas_recvGlobalModelSignal];
        @statistic[flaas_recvGlobalModelSignal](title="flaas_recvGlobalModelSignal"; unit=""; source="flaas_recvGlobalModelSignal"; record=vector; interpolationmode=none;);
        @signal[flaas_sendLocalModelSignal];
        @statistic[flaas_sendLocalModelSignal](title="flaas_sendLocalModelSignal"; unit=""; source="flaas_sendLocalModelSignal"; record=vector; interpolationmode=none;);
        

        
		
    gates:
	  input mePlatformIn;
	  output mePlatformOut;
	  
	  input socketIn;
	  output socketOut;
                
}