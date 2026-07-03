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

package simu5g.stack.phy.errormodel;

//
// Mutual-information effective SINR mapping error model.
//
simple MiesmErrorModel extends ErrorModel
{
    parameters:
        @class("MiesmErrorModel");

        // Fallback calibration scale for the mutual-information mapping.
        double beta = default(1.0);

        // If true, use the existing PhyPisaData lambda table as calibration data
        // when a positive value is available for the selected CQI/tx mode.
        bool useLambdaCalibration = default(true);
}

