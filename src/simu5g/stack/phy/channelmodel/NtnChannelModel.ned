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

package simu5g.stack.phy.channelmodel;

simple NtnChannelModel extends LteRealisticChannelModel
{
    parameters:
        @class("NtnChannelModel");

        scenario = default("NTN_RURAL");
        fixedLos = default(true);

        string antennaModelModule = default("^.antennaModel");
        double polarizationMismatchLoss @unit(dB) = default(-3dB);

        // Interference from other cells is not modelled on a satellite link, in either
        // direction, and enabling it aborts the run rather than producing a plausible wrong
        // number. Both mechanisms are missing the same ingredient -- a propagation-delay
        // term -- but on two different code paths, so each has its own opt-out.
        //
        // UPLINK. Interfering uplink transmissions are looked up in
        // Binder::getUlTransmissionMap(), which keeps a two-slot window around "now" --
        // written by each UE when it transmits, read by the receiver when it decodes.
        // Terrestrially those two instants coincide; over a satellite link they are hundreds
        // of slots apart, so the map reports UEs that transmitted recently rather than UEs
        // whose signals are arriving together. That is a wrong neighbour set, not an
        // imprecise one, and no amount of margin fixes it.
        //
        // What it needs is that map keyed by arrival slot at the receiver, which is item 6
        // of ntn-implementation.md. The reception slot each grant is booked for -- see
        // ~NtnNrMacGnb -- is already exactly that key, so the geometry does not have to be
        // worked out again.
        //
        // Note the map is still POPULATED in NTN runs, because LtePhyUe writes it whenever
        // D2D interference is enabled and that defaults to true. Writing it is harmless;
        // only reading it is wrong, which is why the check is here and not on the write.
        bool ntnAllowUplinkInterference = default(false);

        // DOWNLINK. computeDownlinkInterference() compares this cell's current- or
        // previous-slot resource-block booking against every OTHER cell's own booking
        // (LteMacEnb::getDlBandStatus()/getDlPrevBandStatus(), read directly from each
        // gNodeB's allocator rather than through the Binder), with the same unstated
        // assumption as the uplink case: a booking from one slot ago is what is arriving at
        // the UE now. A satellite's downlink transmission from one slot ago is not what is
        // arriving now.
        //
        // Currently inert regardless of this guard: the branch's one-gateway-one-satellite
        // restriction (item 5) means every NTN scenario today has exactly one NR gNodeB, so
        // the interfering-cell loop in computeDownlinkInterference() finds nothing to
        // iterate over even when this fires.
        bool ntnAllowDownlinkInterference = default(false);
}
