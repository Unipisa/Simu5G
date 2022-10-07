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

package simu5g.nodes.mec.bgMecAppManager;


simple BgMecAppManager
{
    parameters:
        bool fromTraceFile = default(true);
        bool admissionControl = default(false);        
        string traceFileName = default("");
       	double snapshotPeriod = default(-1); // if <= 0 read the timestamp in the tracefile!
	    string mecHostList = default("mecHost"); //comma-space separatedList of MecHosts
        
        // if automatic == true
        double deltaTime @unit(s) = default(1s);
        int maxBgMecApps = default(10);
        double defaultRam @unit(B) = default(100MB);
        double defaultDisk @unit(B) = default(100MB);
        double defaultCpu  = default(5000);
        double centerX @unit(m);
        double centerY @unit(m);
        double centerZ @unit(m) = 0m;
        double radius @unit(m) ;
        
        double mecHostActivation @unit(s) = default(1s);
       
        @display("i=abstract/penguin");
}
