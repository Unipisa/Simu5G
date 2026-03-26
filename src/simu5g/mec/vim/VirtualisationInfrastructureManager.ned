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

package simu5g.mec.vim;


//
// Manages the computing resources currently allocated
// to MEC applications hosted by a MEC host. It is also used by MEC applications in case they
// want to compute a processing delay based on such resource usage.
//
simple VirtualisationInfrastructureManager
{
    parameters:
        @display("i=block/control");

        string binderModule = default("binder");
        int mp1Port = default(10021);
        string scheduling @enum(segregation,fair) = default("segregation");

    gates:
        input virtualisationManagerIn;
        output virtualisationManagerOut;
}



