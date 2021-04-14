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

package simu5g.stack.d2dModeSelection;

//
// Interface for D2D Mode Selection module
//
moduleinterface D2DModeSelection {
    parameters:
        @display("i=block/cogwheel");
}

//
// Base module for the D2D Mode Selection
//
// Extends this module for implementing new mode selection policies.
// It runs periodically the mode selection algorithm and, for flows 
// that must be switched, sends a notification to the transmitting 
// endpoint of those flows
//
simple D2DModeSelectionBase like D2DModeSelection
{
    parameters:
        double modeSelectionPeriod @unit(s) = default(1s);
}

//
// D2DModeSelectionBestCqi module
// 
// For each D2D pair, selects the mode having the best CQI
//
simple D2DModeSelectionBestCqi extends D2DModeSelectionBase
{
    parameters:
        @class("D2DModeSelectionBestCqi");
}