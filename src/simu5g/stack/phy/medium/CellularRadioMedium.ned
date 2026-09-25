//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.phy.medium;

//
// The radio medium of a cellular network: the network-level module that is to
// own signal propagation, the per-link channel realization, interference and
// the SINR, while the nodes keep their antennas, receivers and reception
// decisions. It is not used by the model yet; the PHYs and their channel models
// still do all of this. Exactly one instance is expected in the network, named
// radioMedium.
//
module CellularRadioMedium
{
    parameters:
        @class(CellularRadioMedium);
        @display("i=misc/sun");
}
