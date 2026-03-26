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

package simu5g.mec.platform;

//
// Defines the structure to be implemented by every
// MEC Application that supports standard lifecycle management via ~Ualcmp and
// ~MecOrchestrator (i.e., MEC applications that can be instantiated dynamically
// by User Equipment). See, for example, the ~MecResponseApp module.
//
moduleinterface IMecApp
{
    parameters:
        @display("i=block/app");
        int mecAppId;
        int mecAppIndex;

        string mp1Address;
        int mp1Port;

		//resource required infos
        double requiredRam @unit("B");
        double requiredDisk @unit("B");
        double requiredCpu;
}



