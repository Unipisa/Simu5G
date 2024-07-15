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

package simu5g.apps.mec.MecApps.MecRequestBackgroundApp;

import simu5g.apps.mec.MecApps.MecAppBase;


simple MecRequestBackgroundApp extends MecAppBase
{
    parameters:
        @class(MecRequestBackgroundApp);
        int numberOfApplications = default(1);
        bool allocateHostResources = default(false);

        int packetSize @unit(B) = default(10B);

        mecAppId = default(0); // it will be override by getId() in initialize
        mecAppIndex = default(0);
        localUePort = default(0); // not used in MecRequestBackgroundGeneratorApp

        double lambda @unit(s) = default(10ms);

        //resource required info
        requiredRam = default(10MB);
        requiredDisk = default(100MB);
        requiredCpu = default(15000);

        // IApp parameters
        int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        bool logger = default(false);

        string localAddress = default(""); // may be left empty ("")
        int localPort = default(-1); // port number to listen on
        string connectAddress = default("");  // server address (may be symbolic)
        int connectPort = default(1000); // port number to connect to
        double startTime @unit(s) = default(1s); // time first session begins
        double stopTime @unit(s) = default(-1s);  // time of finishing sending, negative values mean forever

        @display("i=block/app");
}
