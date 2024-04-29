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

package simu5g.apps.mec.MecRequestResponseApp;

import simu5g.apps.mec.MecApps.MecAppBase;


//
//  This is a MEC app that ... TBC
//
simple MECResponseApp extends MecAppBase
{
    parameters:
        @class(MecResponseApp);
        @display("i=block/app;is=s");

        int responsePacketSize @unit(B) = default(50B);

        int minInstructions = default(9000000);
        int maxInstructions = default(11000000);

        // IApp parameters
        int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
}
