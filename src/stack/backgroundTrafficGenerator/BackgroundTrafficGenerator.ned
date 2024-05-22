
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

//
// The BackgroundTrafficGenerator module generates background traffic for LTE simulations. 
// This module can be configured with a specific number of background UEs (numBgUes) 
// and a traffic manager type (bgTrafficManagerType). It supports parameters such as 
// enabling periodic CQI updates (enablePeriodicCqiUpdate), using probabilistic CQI 
// (useProbabilisticCqi), and computing average interference (computeAvgInterference).
//
module BackgroundTrafficGenerator
{
    parameters:
        @display("i=block/source");
        int numBgUes = default(0);
        bool enablePeriodicCqiUpdate = default(true);
        bool useProbabilisticCqi = default(false);
        bool computeAvgInterference = default(false);
        *.computeAvgInterference = this.computeAvgInterference;
        *.enablePeriodicCqiUpdate = this.enablePeriodicCqiUpdate;
        *.useProbabilisticCqi = this.useProbabilisticCqi;
        *.numBgUes = this.numBgUes;

    submodules:
        bgUE[numBgUes]: BackgroundUe {
            @display("p=77,118");
        }
        manager: <default("BackgroundTrafficManager")> like IBackgroundTrafficManager {
            @display("p=77,41");
        }
}

