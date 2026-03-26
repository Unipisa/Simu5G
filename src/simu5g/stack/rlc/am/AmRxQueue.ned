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


package simu5g.stack.rlc.am;

//
// Receive Buffer of RLC AM
//
simple AmRxQueue
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        string amModule = default("^.am");
        string binderModule = default("binder");
        int rxWindowSize = default(200);
        double ackReportInterval @unit(s) = 0.10s;
        double statusReportInterval @unit(s) = 0.20s;
        double timeout @unit(s) = default(1s);            // Timeout for RX Buffer
}

//
// Entities for the RLC module
//

