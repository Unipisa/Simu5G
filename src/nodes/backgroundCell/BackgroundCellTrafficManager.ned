
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

import simu5g.stack.backgroundTrafficGenerator.BackgroundTrafficManagerBase;


//
// This is the traffic manager for background UEs within a
// background cell, which does not have the protocol stack
//
// This manager handles the traffic scheduling and control
// for background user equipments (UEs) by interfacing with
// the background cell's components such as the scheduler,
// AMC, and channel model. It provides functionalities to manage
// resource blocks, compute CQI based on the SINR, handle
// RAC signaling, and calculate received power.
//
simple BackgroundCellTrafficManager extends BackgroundTrafficManagerBase
{
    parameters:
        @display("i=block/control");
        @class("BackgroundCellTrafficManager");
        string bgSchedulerModule = default("^.^.bgScheduler");
        bool enablePeriodicCqiUpdate;
        bool computeAvgInterference;
}
