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

//
// NR NIC for NTN gNodeBs. It keeps the standard NR MAC and isolates the wired
// NTN fronthaul gates to NTN-specific base-station modules.
//
module NtnNrNic extends NrNicEnb
{
    parameters:
        ip2nic.typename = default("simu5g.stack.ip2nic.NtnIp2Nic");
        mac.typename = default("NrMacGnb");
        phy.typename = default("NtnPhyGnb");

        // Must match the served UEs' profile: RLC is a peer protocol, and a gNodeB whose
        // reassembly timer is shorter than its UEs' is worse than either alone. Declared
        // under the same name and default as on ~NtnNrNicUe and on the NTN RLC entity
        // types, so one "**.ntnOrbitProfile" assignment covers the whole scenario.
        string ntnOrbitProfile @enum("GEO","LEO") = default("GEO");

        // NR bearers over the satellite path get the NTN-dimensioned RLC timers; see
        // ~NtnNrRlcAmEntity for the derivation from TR 38.821 and TS 38.331.
        rrc.bearerManagement.nrRlcAmEntityModuleType = default("simu5g.stack.rlc.NtnNrRlcAmEntity");
        rrc.bearerManagement.nrRlcUmEntityModuleType = default("simu5g.stack.rlc.NtnNrRlcUmEntity");

        // RRC re-establishment. The terrestrial 100ms default is well inside one satellite
        // round trip, so a re-establishment attempt would time out before its response
        // could arrive. ms2000 is the largest TS 38.331 T301 enumerated value.
        rrc.bearerManagement.t301 = default(this.ntnOrbitProfile == "GEO" ? 2000ms : 1000ms);

    gates:
        input ntnIn @loose;
        output ntnOut @loose;

    connections:
        ntnIn --> phy.ntnIn;
        phy.ntnOut --> ntnOut;
}
