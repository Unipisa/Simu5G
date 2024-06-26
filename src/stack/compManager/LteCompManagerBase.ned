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

simple LteCompManagerBase like LteCompManager
{
    parameters:
        @display("i=block/cogwheel");
        @class("LteCompManagerBase");

        string compNodeType @enum(COMP_CLIENT,COMP_CLIENT_COORDINATOR,COMP_COORDINATOR) = default("COMP_CLIENT");
        int coordinatorId;							           // id of the coordinator node
        string clientList = default("");                       // list of ids of client nodes, separated by blank spaces
        double coordinationPeriod @unit(s) = default(0.001s);

        @signal[compReservedBlocks];
        @statistic[compReservedBlocks](title="CoMP Reserved Blocks"; unit=""; source="compReservedBlocks"; record=mean,vector);

    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
}
