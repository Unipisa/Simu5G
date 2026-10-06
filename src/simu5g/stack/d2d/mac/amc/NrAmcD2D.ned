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

import simu5g.stack.mac.amc.NrAmc;

//
// NR Adaptive Modulation and Coding (AMC) module with D2D support. It is the
// D2D-capable variant of ~NrAmc, wired in by the D2D NR NICs via the MAC's
// amcType parameter.
//
simple NrAmcD2D extends NrAmc
{
    parameters:
        @class(NrAmcD2D);
        string d2dBinderModule = default("d2dBinder");  // The network's ~D2dBinder, which holds the D2D state
}
