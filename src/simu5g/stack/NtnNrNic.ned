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

        // Grant timing and uplink reception measurement; see ~NtnNrMacGnb.
        mac.typename = default("NtnNrMacGnb");
        phy.typename = default("NtnPhyGnb");

        // NR bearers over the satellite path get the NTN-dimensioned RLC timers, each derived
        // from this cell's own round-trip delay; see ~NtnNrRlcAmEntity for the derivation from
        // TR 38.821 and TS 38.331. This must match the redirect ~NtnNrNicUe makes: RLC is a peer
        // protocol, and a gNodeB whose reassembly timer is shorter than its UEs' is worse than
        // either value alone. Both ends read the same cell round-trip delay, so they agree by
        // construction.
        rrc.bearerManagement.nrRlcAmEntityModuleType = default("simu5g.stack.rlc.NtnNrRlcAmEntity");
        rrc.bearerManagement.nrRlcUmEntityModuleType = default("simu5g.stack.rlc.NtnNrRlcUmEntity");

        // Must match the served UEs' values: LteHarqBufferRx is sized from the receiver's
        // own harqProcesses while the transmitter picks an ACID from its own, so a mismatch
        // indexes out of range, and feedback is a peer agreement -- a transmitter that has
        // stopped waiting must be paired with a receiver that has stopped sending. See
        // ~NtnNrMacUe for the derivation and the measured effect of each direction.
        mac.harqProcesses = default(32);
        mac.harqFeedbackEnabledDl = default(false);
        mac.harqFeedbackEnabledUl = default(true);

        // RRC re-establishment. The terrestrial 100ms default is well inside one satellite
        // round trip, so a re-establishment attempt would time out before its response
        // could arrive. ms2000 is the largest TS 38.331 T301 enumerated value; see ~NtnNrNicUe
        // for why this one value is not geometry-derived.
        rrc.bearerManagement.t301 = default(2000ms);

    gates:
        input ntnIn @loose;
        output ntnOut @loose;

    connections:
        ntnIn --> phy.ntnIn;
        phy.ntnOut --> ntnOut;
}
