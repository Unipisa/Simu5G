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

package simu5g.stack.compManager;

import simu5g.x2.IX2User;

//
// The LteCompManager interface defines the structure for the CoMP Manager.
// It extends the X2User interface and provides the necessary parameters and
// configurations for managing Coordinated Multi-Point (CoMP) communications
// in LTE networks.
//
// @neddoc
// The `LteCompManager` module interface provides a blueprint for implementing
// the CoMP (Coordinated Multi-Point) manager functionality in the LTE network stack.
// It extends the `X2User` interface, inheriting its capabilities and providing additional
// hooks for managing CoMP-specific operations. The interface comes with a built-in
// display icon for visual representation in network simulations.
//
moduleinterface ILteCompManager extends IX2User
{
    parameters:
        @display("i=block/cogwheel");
}

