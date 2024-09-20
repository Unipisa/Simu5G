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
// The NRNicEnb module serves as a network interface that provides 5G connectivity
// within a gNodeB. It implements the data and signaling plane of the NR (New Radio) stack,
// which encompasses the PDCP, RLC, MAC, and PHY layers. The parameters and submodules
// facilitate the customization of the gNodeB's behavior, including its support for
// D2D communication, CoMP (Coordinated MultiPoint), or dual connectivity.
//
module NRNicEnb extends LteNicEnbD2D
{
    parameters:
        pdcpRrc.typename = default("NRPdcpRrcEnb");
        mac.typename = default("NRMacGnb");
        LteChannelModelType = default("NRChannelModel_3GPP38_901");
        @display("bgb=694,443");
        packetFlowManager.typename = default("NRPacketFlowManagerGnb");
}

