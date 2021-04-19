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

package simu5g.nodes.backgroundCell;

import simu5g.stack.backgroundTrafficGenerator.IBackgroundTrafficManager;
import simu5g.stack.backgroundTrafficGenerator.BackgroundTrafficManager;

// BackgroundCellTrafficManager
//
// This is the traffic manager for background UEs within a
// background cell, which does not have the protocol stack  
simple BackgroundCellTrafficManager extends BackgroundTrafficManager like IBackgroundTrafficManager
{
    parameters:
        @display("i=block/control");
        @class("BackgroundCellTrafficManager");
}