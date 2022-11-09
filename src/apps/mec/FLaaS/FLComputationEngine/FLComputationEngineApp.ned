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

package simu5g.apps.mec.FLaaS.FLComputationEngine;

import inet.applications.contract.IApp;
import simu5g.nodes.mec.MECPlatform.IMECApp;
//import simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase.IMECService;

// TODO describe it

simple FLComputationEngineApp like IMECApp, IApp
{
    parameters:
        @display("i=block/app;is=s");
  		@dynamic(true);
  		
  		// IMECApp parameters
        int packetSize = default(10);
        string interfaceTableModule = default("");
        
        int mecAppIndex;
        int mecAppId;
        int localUePort;
        
        string mp1Address = default("");
        int mp1Port = default(10021);
        
        volatile double roundDuration @unit("s") = default(uniform(60s, 90s, 3));
        volatile double modelAggregationDuration @unit("s") = default(uniform(1s, 2s, 3));
//        double roundDuration @unit("s") = uniform(60s, 90s);
//        double modelAggregationDuration @unit("s") = uniform(1s, 2s);

        int minLearners;// = 2;
        int localModelTreshold = int(minLearners*0.75);
        int trainingMode @enum(SYNCHRONOUS, ASYNCHRONOUS, BOTH, ONE_SHOT) = default(3);
        double modelDimension @unit("B") = default(240kB);
        string controllerModuleName = "";
        double repeatTime @unit("s") = default(1s); 
        
		 //resource required info
        double requiredRam @unit("B");
        double requiredDisk @unit("B");
        double requiredCpu;
        
        double instantiationTime @unit("s") = 1s;
        
		// IApp parameters
		int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        
        // stats
        @signal[flaas_roundLifeCycle];
        @statistic[flaas_roundLifeCycle](title="Round LifeCycle"; unit=""; source="flaas_roundLifeCycle"; record=vector; interpolationmode=none;); 
        
		@signal[flaas_startRoundSignal];
        @statistic[flaas_startRoundSignal](title="startRoundSignal"; unit=""; source="flaas_startRoundSignal"; record=vector; interpolationmode=none;);
        @signal[flaas_sentGlobalModelSignal];
        @statistic[flaas_sentGlobalModelSignal](title="flaas_sentGlobalModelSignal"; unit=""; source="flaas_sentGlobalModelSignal"; record=vector; interpolationmode=none;);
        @signal[flaas_recvLocalModelSignal];
        @statistic[flaas_recvLocalModelSignal](title="flaas_recvLocalModelSignal"; unit=""; source="flaas_recvLocalModelSignal"; record=vector; interpolationmode=none;);
		
		@signal[flaas_ulTimeSignal];
        @statistic[flaas_ulTimeSignal](title="flaas_ulTimeSignal"; unit=""; source="flaas_ulTimeSignal"; record=mean,vector; interpolationmode=none;);
		@signal[flaas_dlTimeSignal];
        @statistic[flaas_dlTimeSignal](title="flaas_dlTimeSignal"; unit=""; source="flaas_dlTimeSignal"; record=mean,vector; interpolationmode=none;);
		@signal[flaas_trainingTimeSignal];
        @statistic[flaas_trainingTimeSignal](title="flaas_trainingTimeSignal"; unit=""; source="flaas_trainingTimeSignal"; record=mean,vector; interpolationmode=none;);
		
		
		@signal[flaas_trainingDurationSignal];
        @statistic[flaas_trainingDurationSignal](title="flaas_trainingDurationSignal"; unit=""; source="flaas_trainingDurationSignal"; record=mean,vector; interpolationmode=none;);
		@signal[flaas_trainingDurationSignal_connection];
        @statistic[flaas_trainingDurationSignal_connection](title="flaas_trainingDurationSignal_connection"; unit=""; source="flaas_trainingDurationSignal_connection"; record=mean,vector; interpolationmode=none;);
		
        
		
    gates:  
	  input socketIn;
	  output socketOut;               
}
