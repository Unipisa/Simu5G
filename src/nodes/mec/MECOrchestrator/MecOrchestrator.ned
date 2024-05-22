
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
// This module oversees the MEC orchestrator's functionalities within a MEC system.
// Not adhering strictly to ETSI standard APIs, it leverages OMNeT++ features to manage the lifecycle operations of MEC applications.
// It communicates with the LCM proxy via connections and is configured to administer MEC hosts associated with the MEC system based on parameters.
// Key functionalities provided include:
//   - MEC app instantiation
//   - MEC app termination
//   - MEC app run-time onboarding
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
