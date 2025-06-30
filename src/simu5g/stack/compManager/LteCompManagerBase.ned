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

//
 // This module serves as the base class for Coordinated Multi-Point (CoMP) manager modules in LTE networks.
 // It provides essential functionalities and parameters needed to manage CoMP operations.
 // To implement a specific CoMP algorithm, extend this class and override its pure virtual methods.
 //
// ALTERNATIVE:
//
// LteCompManagerBase: Base class for CoMP manager modules. This module serves as the
// foundational component for developing and integrating different CoMP algorithms
// within the simu5g framework. Extend this module and override its virtual methods
// to implement specific CoMP algorithms.
//
// Parameters:
// - compNodeType: Specifies the type of CoMP node (client, client and coordinator, or coordinator only).
// - coordinatorId: The identifier for the coordinator node.
// - clientList: A space-separated list of client node identifiers.
// - coordinationPeriod: The time interval between two coordination instances.
//
// Gates:
// - x2ManagerIn: Input gate for connections to the X2 Manager.
// - x2ManagerOut: Output gate for connections to the X2 Manager.
//
// Signals:
// - compReservedBlocks: Signal to track the number of reserved blocks for CoMP operations.
//
// Statistics:
// - compReservedBlocks: Statistics related to CoMP reserved blocks, recorded as mean and vector.
//
simple LteCompManagerBase like ILteCompManager
{
    parameters:
        @display("i=block/cogwheel");
        @class("LteCompManagerBase");

        string compNodeType @enum(COMP_CLIENT,COMP_CLIENT_COORDINATOR,COMP_COORDINATOR) = default("COMP_CLIENT");
        int coordinatorId;							           // id of the coordinator node
        object clientList = default([]);                       // list of ids of client nodes
        double coordinationPeriod @unit(s) = default(0.001s);
        string binderModule = default("binder");

        @signal[compReservedBlocks];
        @statistic[compReservedBlocks](title="CoMP Reserved Blocks"; unit=""; source="compReservedBlocks"; record=mean,vector);

    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
}
