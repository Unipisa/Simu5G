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

        // Orbit whose round-trip delay the protocol timers are dimensioned for. Declared
        // under the same name and default here, on ~NtnNrNic, and on the NTN RLC entity
        // types, so that one "**.ntnOrbitProfile" assignment configures every timer on
        // both ends of a bearer. RLC is a peer protocol: a UE and a gNodeB on different
        // profiles is worse than either profile alone.
        string ntnOrbitProfile @enum("GEO","LEO") = default("GEO");

        // NR bearers over the satellite path get the NTN-dimensioned RLC timers. The
        // entity types are resolved by BearerManagement at bearer setup, so redirecting
        // these strings is all that is needed -- no NIC topology changes. The entities
        // themselves are created with the NIC as parent, not under bearerManagement, and
        // carry their own ntnOrbitProfile default.
        rrc.bearerManagement.nrRlcAmEntityModuleType = default("simu5g.stack.rlc.NtnNrRlcAmEntity");
        rrc.bearerManagement.nrRlcUmEntityModuleType = default("simu5g.stack.rlc.NtnNrRlcUmEntity");

        // The NR MAC gets the NTN-dimensioned random-access and buffer-status-report
        // counters; see ~NtnNrMacUe. Only the NR leg is retyped -- the LTE-leg "mac"
        // carries no NTN traffic. Unlike RLC these counters are not a peer protocol: only
        // the UE counts them, so ~NtnNrNic needs no matching change.
        nrMac.typename = default("NtnNrMacUe");

        // RRC re-establishment. The terrestrial 100ms default is well inside one satellite
        // round trip, so a re-establishment attempt would time out before its response
        // could arrive. ms2000 is the largest TS 38.331 T301 enumerated value.
        rrc.bearerManagement.t301 = default(this.ntnOrbitProfile == "GEO" ? 2000ms : 1000ms);

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
