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

package simu5g.stack.mac.scheduler;

//
// LTE eNB uplink scheduler module.
//
simple LteSchedulerEnbUl like ILteSchedulerEnbUl
{
    parameters:
        @class(SchedulerDummy);
		@display("i=block/cogwheel");
}
