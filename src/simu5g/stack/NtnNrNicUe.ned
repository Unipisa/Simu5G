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

package simu5g.stack;

import simu5g.stack.phy.antennamodel.IAntennaModel;
import simu5g.stack.phy.channelmodel.ILteChannelModel;

//
// NR NIC for User Equipments served through a transparent NTN path. It replaces
// the PHY layers with NTN-specific modules, which reach the serving gNodeB via
// the satellite service link, and adds the channel and antenna models used to
// evaluate that link.
//
module NtnNrNicUe extends NrNicUe
{
    parameters:
        string ntnChannelModelType = default("NtnChannelModel");
        string ntnAntennaModelType = default("IsotropicAntennaModel");

        int numNtnCarriers = default(1);

        // both PHY layers are NTN-capable, as they share the NTN channel and antenna models
        phy.typename = default("NtnPhyUe");
        nrPhy.typename = default("NtnPhyUe");

        // NR bearers over the satellite path get the NTN-dimensioned RLC timers, each derived
        // from this cell's own round-trip delay. The entity types are resolved by
        // BearerManagement at bearer setup, so redirecting these strings is all that is needed
        // -- no NIC topology changes. Both ends of a bearer must use them: RLC is a peer
        // protocol, and a receiver whose reassembly timer is shorter than its transmitter's poll
        // interval is worse than either value alone. ~NtnNrNic makes the matching redirect.
        rrc.bearerManagement.nrRlcAmEntityModuleType = default("simu5g.stack.rlc.NtnNrRlcAmEntity");
        rrc.bearerManagement.nrRlcUmEntityModuleType = default("simu5g.stack.rlc.NtnNrRlcUmEntity");

        // The NR MAC gets the NTN-dimensioned random-access and buffer-status-report
        // counters; see ~NtnNrMacUe. Only the NR leg is retyped -- the LTE-leg "mac"
        // carries no NTN traffic. Unlike RLC these counters are not a peer protocol: only
        // the UE counts them, so ~NtnNrNic needs no matching change.
        nrMac.typename = default("NtnNrMacUe");

        // RRC re-establishment. The terrestrial 100ms default is well inside one satellite
        // round trip, so a re-establishment attempt would time out before its response
        // could arrive. ms2000 is the largest TS 38.331 T301 enumerated value, and is used for
        // every orbit rather than being derived: in this model t301 is a resume delay after
        // radio link failure rather than a protocol deadline, so an over-long value costs a
        // little extra outage and nothing else. Deriving it would need an NTN BearerManagement,
        // because t301 is read into a private member at INITSTAGE_LOCAL.
        rrc.bearerManagement.t301 = default(2000ms);

    submodules:
        // NTN Channel Model
        ntnChannelModel[numNtnCarriers]: <ntnChannelModelType> like ILteChannelModel {
            @display("p=620,314");
            antennaModelModule = "^.ntnAntennaModel";
        }
        // NTN Antenna Model
        ntnAntennaModel: <ntnAntennaModelType> like IAntennaModel {
            @display("p=620,226");
        }
}
