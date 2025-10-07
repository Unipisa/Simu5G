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

package simu5g.stack.compManager.compManagerProportional;

import simu5g.stack.compManager.LteCompManagerBase;

//
// Implements a CoMP (Coordinated Multi-Point) Proportional Scheduling
// algorithm. It allows for coordinated scheduling among multiple
// eNodeBs by calculating the required Resource Block (RB) allocation
// proportionally based on user load.
// In this structure, the module can function as either a master or a
// slave node. Slave nodes send their RB requests to the master node,
// which then processes these requests by partitioning the available
// bandwidth and subsequently informs each slave node about the
// available RBs for transmission.
//
simple LteCompManagerProportional extends LteCompManagerBase
{
    parameters:
        @display("i=block/cogwheel");
        @class("LteCompManagerProportional");
}
