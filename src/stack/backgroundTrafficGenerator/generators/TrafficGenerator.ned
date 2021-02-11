// 
//                           Simu5G
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.stack.backgroundTrafficGenerator.generators;

import inet.mobility.contract.IMobility;

moduleinterface ITrafficGenerator
{
    parameters:
        @display("i=block/source");
}

simple TrafficGeneratorBase like ITrafficGenerator 
{
    parameters:
        @display("i=block/source");
        @class("TrafficGeneratorBase");
        
        double startTimeDl @unit("s") = default(0s);
        double startTimeUl @unit("s") = default(0s);
        
        int headerLen @unit(B) = default(33B); // TODO check here
         
        volatile double period @unit("s") = default(uniform(10ms,50ms));
        volatile int size @unit(B) = default(uniform(20B,100B));
}

//# add here your custom generators, by extending the base module

