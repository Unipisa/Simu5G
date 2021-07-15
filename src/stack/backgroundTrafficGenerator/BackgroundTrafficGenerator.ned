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

package simu5g.stack.backgroundTrafficGenerator;

import simu5g.stack.backgroundTrafficGenerator.BackgroundTrafficManager;

module BackgroundTrafficGenerator
{
    parameters:
        @display("i=block/source");
        int numBgUes = default(0);
        string bgTrafficManagerType = default("BackgroundTrafficManager");
        bool enablePeriodicCqiUpdate = default(true);
        bool useProbabilisticCqi = default(false);
        bool computeAvgInterference = default(false);
        
    submodules:
        bgUE[numBgUes]: BackgroundUe {
            @display("p=77,118");
        }
        manager: <bgTrafficManagerType> like IBackgroundTrafficManager {
            @display("p=77,41");
        }
}

