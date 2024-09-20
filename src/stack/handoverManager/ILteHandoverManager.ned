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

package simu5g.stack.handoverManager;

import simu5g.x2.IX2User;

//
// Interface for the Handover Manager. This module extends the ~X2User module interface
// to define the basic contract that any handover manager must adhere to.
//
moduleinterface ILteHandoverManager extends IX2User
{
    parameters:
        @display("i=block/cogwheel");
}

