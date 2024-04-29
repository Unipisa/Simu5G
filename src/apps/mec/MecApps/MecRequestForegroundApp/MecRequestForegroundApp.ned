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
// This app simulates a MEC app that sends GET requests to the Location Service
// every 500ms. It used when the user wants to evaluate MEC host related statistics,
// without the need of using UEs to instantiate MEC apps through the MEC orchestrator.
// It has been used, for instance, to evaluate the loadgenerator model of the MEC service
// to see the execution time differences.
//
simple MecRequestForegroundApp extends MecAppBase
{
    parameters:
        @class(MecRequestForegroundApp);
        bool allocateHostResources = default(false);

        int packetSize @unit(B) = default(10B);
        double lambda @unit(s) = default(10ms);

        // IApp parameters
        int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        bool logger = default(false);
}
