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

//
// TODO auto-generated type
//
package simu5g.rdoexperiment;

//
// RDO-T Module for Resilient Distributed Optimization with Trusted Agents
// Based on "Resilient Distributed Optimization Algorithm Against Adversarial Attacks"
// by Chengcheng Zhao, Jianping He, and Qing-Guo Wang
//
simple RDOManager
{
    parameters:
        @display("i=block/cogwheel");
        string configFile = default("rdo_config.csv");  // Path to RDO configuration file
        bool validateCDS = default(true);              // Validate that trusted nodes form a connected dominating set
        
    gates:
        // No gates needed as communication happens through UeApp
}

//
// RDO-T Network Definition
// This is a composite module that incorporates RDO into the existing network
//
module RDO_T
{
    parameters:
        @display("i=block/network2");
        bool enabled = default(true);                  // Whether RDO is enabled
        string configFile = default("rdo_config.csv"); // Path to RDO configuration file
        
    submodules:
        rdoManager: RDOManager {
            parameters:
                configFile = configFile;
                @display("p=50,50");
        }
}

//
// Metrics Collector for gNBs with RDO metrics
//
simple RDOMetricsCollector
{
    parameters:
        @display("i=block/table");
        bool enabled = default(true);                  // Whether metrics collection is enabled
        int gnbId;                                     // ID of the gNB
        string scenario = default("rdo");             // Simulation scenario
        string metricsFile;                           // Path to output metrics file
        double interval @unit(s) = default(1s);       // Collection interval
}
