//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.d2d.rrc;

import simu5g.stack.rrc.HandoverController;

//
// D2D-capable variant of the ~HandoverController: the leg reports itself D2D-capable
// when it connects to a base station (if its PHY is), which is where the D2D side
// of its handovers is handled (~ConnectionControlEnbD2D), and a torn-down sidelink
// bearer's DRB identity returns to the network-wide sidelink pool (~D2dBinder).
// Selected via 'handoverController.typename' / 'nrHandoverController.typename' by
// the D2D-capable UE NICs.
//
simple HandoverControllerD2D extends HandoverController
{
    parameters:
        @class(HandoverControllerD2D);
}
