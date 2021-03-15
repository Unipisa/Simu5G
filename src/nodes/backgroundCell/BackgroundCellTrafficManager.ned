// 
//                           Simu5G
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.nodes.backgroundCell;

import lte.stack.backgroundTrafficGenerator.IBackgroundTrafficManager;
import lte.stack.backgroundTrafficGenerator.BackgroundTrafficManager;

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