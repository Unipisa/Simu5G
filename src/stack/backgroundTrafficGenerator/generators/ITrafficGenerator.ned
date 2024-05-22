
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
// within the background traffic generation framework of the Simu5G project.
// Implementing modules should provide mechanisms for generating traffic
// with specific parameters such as start times, packet sizes, and periods.
//
moduleinterface ITrafficGenerator
{
    parameters:
        @display("i=block/source");
}

