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
// D2DModeSelectionBestCqi module
//
// This module implements the mode selection algorithm for D2D communications
// that selects the mode based on the best Channel Quality Indicator (CQI).
// It evaluates the CQI for each D2D-capable flow and selects the communication
// mode that provides the best performance in terms of achievable data rate.
// The primary goal is to optimize data transmission quality and efficiency
// by dynamically switching between modes based on CQI measurements.
//
simple D2DModeSelectionBestCqi extends D2DModeSelectionBase
{
    parameters:
        @class("D2DModeSelectionBestCqi");
}
