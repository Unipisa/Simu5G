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


package simu5g.stack.backgroundTrafficGenerator;

//
// This module interface defines a common set of parameters and functionality
// for traffic managers handling background UEs' (User Equipments) traffic
// within the simulation. Implementing modules can specify various traffic 
// models and strategies for generating and managing background traffic, 
// which can be used to test network performance and behavior under different 
// traffic conditions with minimal impact on the simulator performance.
//
moduleinterface IBackgroundTrafficManager
{
    parameters:
        @display("i=block/control");
}

