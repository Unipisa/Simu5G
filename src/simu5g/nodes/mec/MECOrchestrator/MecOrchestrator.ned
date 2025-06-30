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

package simu5g.nodes.mec.MECOrchestrator;

//
// This module models the functionalities of the MEC orchestrator within a Multi-access Edge
// Computing (MEC) system. Specifically, it manages the dynamic instantiation and
// termination of MEC applications on the MEC hosts belonging to the MEC system. Upon
// receiving a MEC app instantiation request from the ~UALCMP module, this module is
// responsible for selecting the MEC host on which the MEC application is to be instantiated, according
// to a configurable policy.
//
simple MecOrchestrator
{
    parameters:
        @display("i=device/mainframe;bgb=1006,692");
        string binderModule = default("binder");

        string selectionPolicy @enum(MecHostBased,MecServiceBased,AvailableResourcesBased) = default("MecServiceBased"); // available policies: "MecHostBased", "MecServiceBased", "AvailableResourcesBased"
        int mecHostIndex = default(0); // to be used with the MecHostBased policybased
        object mecHostList = default([]);
        object mecApplicationPackageList = default([]);

        // times to simulate lifecycle operations
        double onboardingTime @unit(s) = default(50ms);
        double instantiationTime @unit(s) = default(50ms);
        double terminationTime @unit(s) = default(50ms);

    gates:
        output toUALCMP;
        input fromUALCMP;
}
