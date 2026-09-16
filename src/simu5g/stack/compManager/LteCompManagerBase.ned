//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.compManager;

//
// Serves as the base class for Coordinated Multi-Point (CoMP) manager modules in LTE networks.
// It provides essential functionalities and parameters needed to manage CoMP operations.
// To implement a specific CoMP algorithm, extend this class and override its pure virtual methods.
//
simple LteCompManagerBase like ILteCompManager
{
    parameters:
        @display("i=block/cogwheel");
        @class("LteCompManagerBase");

        string compNodeType @enum(COMP_CLIENT,COMP_CLIENT_COORDINATOR,COMP_COORDINATOR) = default("COMP_CLIENT");
        int coordinatorId;  // ID of the coordinator node
        object clientList = default([]);   // Space-separated list of client node identifiers
        double coordinationPeriod @unit(s) = default(0.001s);  // The time interval between two coordination events
        string binderModule = default("binder");

        @signal[compReservedBlocks];
        @statistic[compReservedBlocks](title="CoMP Reserved Blocks"; unit=""; source="compReservedBlocks"; record=mean,vector);

    gates:
        //# Connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
}
