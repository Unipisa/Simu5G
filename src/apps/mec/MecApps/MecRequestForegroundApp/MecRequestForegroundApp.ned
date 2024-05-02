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

package simu5g.apps.mec.MecApps.MecRequestForegroundApp;

import simu5g.apps.mec.MecApps.MecAppBase;

//
// This module implements a MEC application that issues GET requests to the
// ~LocationService every 500ms. This application can be utilized as an "independent MEC app" (see
// ~MecHost module), hence it does not require instantiation via the MEC orchestrator
// upon request from a User Equipment. Essentially, this application provides a quick
// way to generate configurable load to the Location Service and simulate different
// (and potentially more realistic) response times from the latter.
//
// TODO: make the period configurable
//
simple MecRequestForegroundApp extends MecAppBase
{
    parameters:
        @class(MecRequestForegroundApp);
        double lambda @unit(s) = default(10ms);
}
