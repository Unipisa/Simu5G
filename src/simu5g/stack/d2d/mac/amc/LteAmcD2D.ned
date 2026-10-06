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

package simu5g.stack.d2d.mac.amc;

import simu5g.stack.mac.amc.LteAmc;

//
// LTE Adaptive Modulation and Coding (AMC) module with D2D support. It is the
// D2D-capable variant of ~LteAmc, wired in by the D2D LTE NICs via the MAC's
// amcType parameter.
//
simple LteAmcD2D extends LteAmc
{
    parameters:
        @class(LteAmcD2D);
        string d2dBinderModule = default("d2dBinder");  // The network's ~D2dBinder, which holds the D2D state
}
