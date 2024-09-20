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

package simu5g.x2;

//
// This module interface provides the fundamental structure for submodules of the LTE/NR protocol
// stack that need to interact with the ~LteX2Manager to utilize X2 communication to accomplish
// their tasks, such as handover, dual connectivity, and Coordinated Multi-Point.
//
moduleinterface IX2User
{
    parameters:
        @display("i=block/cogwheel");
    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
}
