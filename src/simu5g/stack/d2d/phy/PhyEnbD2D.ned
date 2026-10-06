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


package simu5g.stack.d2d.phy;

import simu5g.stack.phy.PhyEnb;

//
// Extends the PHY functionalities on the eNodeB side to support device-to-device (D2D)
// communications. Specifically, it enables the computation of Channel Status Information (CSI) related
// to the D2D link between two UEs, which still needs to be stored at the eNodeB for scheduling purposes.
//
simple PhyEnbD2D extends PhyEnb
{
    parameters:
        @class("PhyEnbD2D");
        string d2dBinderModule = default("d2dBinder");  // The network's ~D2dBinder, which holds the D2D state
        bool enableD2DCqiReporting = default(true);
}
