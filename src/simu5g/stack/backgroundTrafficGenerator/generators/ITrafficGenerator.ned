//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


package simu5g.stack.backgroundTrafficGenerator.generators;

//
// This module interface defines the contract for traffic generator modules
// within the ~BackgroundTrafficGenerator module. This interface can be implemented
// by other modules to create a custom traffic generator; see ~TrafficGeneratorCbr as
// an example.
//
moduleinterface ITrafficGenerator
{
    parameters:
        @display("i=block/source");
}

