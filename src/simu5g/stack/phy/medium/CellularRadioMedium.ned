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
// decisions. So far it keeps the registry of the radios (every PHY registers
// its own) and delivers broadcast frames, the base station beacons, to the
// radios in range; the channel models still do the rest. Exactly one instance
// is expected in the network, named radioMedium.
//
// The parameters pMax, sat, alpha and carrierFrequency are not part of the
// radio propagation model -- that is the channel model of each PHY -- but they
// set the range of a broadcast: the distance at which a free-space
// transmission at pMax falls to the sat threshold. Beacons drive handover and
// the detection of a lost serving cell.
//
module CellularRadioMedium
{
    parameters:
        double pMax @unit("mW") = default(20mW); // Maximum sending power used for this network (in mW)
        double sat @unit("dBm") = default(-110dBm); // Signal attenuation threshold (in dBm)
        double alpha = default(2); // Path loss coefficient
        double carrierFrequency @unit("Hz") = default(2.4GHz); // Carrier frequency (in Hz)
        @class(CellularRadioMedium);
        @display("i=misc/sun");
}
