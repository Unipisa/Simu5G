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
        // direction, and enabling it aborts the run. Both mechanisms assume a
        // transmission from one or two slots ago is what is arriving now -- true
        // terrestrially, false by hundreds of slots over a satellite link -- on two
        // different code paths, so each has its own opt-out. Fixing this is item 6 of
        // ntn-implementation.md: uplink needs Binder::getUlTransmissionMap() keyed by
        // arrival slot (the reception slot ~NtnNrMacGnb already computes per grant);
        // downlink compares via LteMacEnb::getDlBandStatus()/getDlPrevBandStatus()
        // instead, and is inert today regardless since every NTN scenario has one gNodeB.
        bool ntnAllowUplinkInterference = default(false);
        bool ntnAllowDownlinkInterference = default(false);
}
