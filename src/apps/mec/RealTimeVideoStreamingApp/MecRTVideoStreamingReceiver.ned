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

package simu5g.apps.mec.RealTimeVideoStreamingApp;

import simu5g.apps.mec.MecApps.MecAppBase;


simple MecRTVideoStreamingReceiver extends MecAppBase
{
    parameters:
        @class(MecRTVideoStreamingReceiver);
        @display("i=block/app;is=s");

        int packetSize @unit(B) = default(10B);

        double discardPercentage = 0.8;
        double initialPlayoutDelay @unit(s) = 150ms;
}
