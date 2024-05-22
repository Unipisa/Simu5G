
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
// The `X2User` module interface is a foundational component in the Simu5G framework,
// providing the necessary parameters and gate definitions for modules that need to 
// communicate with the X2 Manager. It is designed to standardize the communication 
// protocols between different modules and the X2 Manager, ensuring efficient and 
// reliable data exchange in LTE network simulations. The module comes with a 
// predefined display icon for easy visualization.
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
