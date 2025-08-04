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

// 
// CarApp.ned
//
// Application module for vehicular nodes that simulates normal message
// transmission and various attack behaviors in V2X communication scenarios.

package simu5g.rdoexperiment;

import inet.applications.contract.IApp;

simple CarApp like IApp
{
    parameters:
        @display("i=block/app");
        @class(CarApp);
        
        //string interfaceTableModule;
        
        // Mobility parameters
        string mobilityFile = default("");  // Path to the mobility trace file
        
        // Attack parameters
        int numFakeIds = default(5);        // Number of fake IDs for Sybil attack
        int dosPacketCount = default(10);   // Number of packets to send in DoS attack
        double attackSeverity = default(1.0); // Severity factor for attacks
        
        // Statistics
        @signal[packetsSent](type=long);
        @signal[attackRate](type=long);
        @signal[positionX](type=double);
        @signal[positionY](type=double);
        @signal[speed](type=double);
        
        @statistic[packetsSent](title="packets sent"; source=packetsSent; record=count,sum,vector);
        @statistic[attackRate](title="attack packet rate"; source=attackRate; record=vector);
        @statistic[positionX](title="X position"; source=positionX; record=vector);
        @statistic[positionY](title="Y position"; source=positionY; record=vector);
        @statistic[speed](title="speed"; source=speed; record=vector);
        
    gates:
        input socketIn @labels(UdpCommand/up);
        output socketOut @labels(UdpCommand/down);
}







//package simu5g.rdoexperiment;
//
//import inet.applications.contract.IApp;
//
////
//// UE Application module for RDO experiment
//// Used to simulate normal behavior and attack scenarios
////
//// CarApp.ned
////
//// Application module for vehicular nodes that simulates normal message
//// transmission and various attack behaviors in V2X communication scenarios.
//
//
//simple CarApp like IApp
//{
//    parameters:
//        @display("i=block/app");
//        @class(CarApp);
//        
//        // Statistics
//        @signal[packetsSent](type=long);
//        @signal[attackRate](type=long);
//        @statistic[packetsSent](title="packets sent"; source=packetsSent; record=count,sum,vector);
//        @statistic[attackRate](title="attack packet rate"; source=attackRate; record=vector);
//        
//    gates:
//        input socketIn @labels(TcpCommand/up);
//        output socketOut @labels(TcpCommand/down);
//}