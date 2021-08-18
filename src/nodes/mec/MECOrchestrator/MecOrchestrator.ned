//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.nodes.mec.MECOrchestrator;

// 
// This module implements the MEC orchestrator of a MEC system.
// It does not follow ETSI compliant APIs, but the it handles the lifecycle operations 
// of the standard by using OMNeT++ features.
// Communications with the LCM proxy occur via connections, while the MEC hosts associated with
// the MEC system (and the MEC orchestrator) are managed with the mecHostList parameter.
// This MEC orchestrator provides:
//   - MEC app instantiation
//   - MEC app termination
//   - MEC app run-time onboarding
// 

simple MecOrchestrator
{
    parameters:

	    string mecHostList = default("");
	    string mecApplicationPackageList = default("");
	    
	    // times to simulate lifecycle operations 
	    double onboardingTime @unit(s) = default(50ms);
	    double instantiationTime @unit(s) = default(50ms);
	    double terminationTime @unit(s) = default(50ms);
	
    gates:
                
        output toLcmProxy;
        input fromLcmProxy;

}