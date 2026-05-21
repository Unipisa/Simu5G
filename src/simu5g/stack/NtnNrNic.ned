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

    gates:
        input ntnIn @loose;
        output ntnOut @loose;

    connections:
        ntnIn --> phy.ntnIn;
        phy.ntnOut --> ntnOut;
}
