//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


package simu5g.common.cellInfo;

//
// Includes data structures and functions used to support local operations
// at the e/gNodeB, such as the cell ID, the set of Component Carriers (CCs) supported
// by the e/gNodeB, and more.
//
// TODO: Remove obsolete parameters
//
simple CellInfo
{
    parameters:
        @display("i=block/control");

        @class("CellInfo");
        //#
        //# General properties
        //#

        // The minimum X-coordinate of the area constraint, unconstrained by default
        double constraintAreaMinX @unit(m) = default(-1m/0);

        // The minimum Y-coordinate of the area constraint, unconstrained by default
        double constraintAreaMinY @unit(m) = default(-1m/0);

        // The maximum X-coordinate of the area constraint, unconstrained by default
        double constraintAreaMaxX @unit(m) = default(1m/0);

        // The maximum Y-coordinate of the area constraint, unconstrained by default
        double constraintAreaMaxY @unit(m) = default(1m/0);

        // Interval for sending broadcast messages (used for handover or DAS)
        double broadcastMessageInterval @unit(s) = default(1s);

        // Type of eNodeB (MICRO / MACRO) - valid only for eNodeBs
        bool microCell = default(false);

        //#
        //# Physical layer properties
        //#

        // Number of sub-carriers per RB in DL
        int rbyDl = default(12);

        // Number of sub-carriers per RB in UL
        int rbyUl = default(12);

        // Number of OFDM symbols per slot in DL
        int rbxDl = default(7);

        // Number of OFDM symbols per slot in UL
        int rbxUl = default(7);

        // Number of pilot Resource Elements per RB in DL
        int rbPilotDl = default(3);

        // Number of pilot Resource Elements per RB in UL
        int rbPilotUl = default(0);

        // Number of signaling symbols for each Resource Block in DL
        int signalDl = default(1);

        // Number of signaling symbols for each Resource Block in UL
        int signalUl = default(1);

        // Number of preferred bands to use (relevant only in PREFERRED mode)
        int numPreferredBands = default(1);

        string binderModule = default("binder");
}
