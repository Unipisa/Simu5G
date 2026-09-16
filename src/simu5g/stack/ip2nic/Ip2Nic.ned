//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//
package simu5g.stack.ip2nic;

//
// Bridges the IP network layer and the cellular protocol stack, handling bidirectional packet flow.
// Extracts flow information from IP headers and attaches it as control tags for the stack.
// In base stations, determines the destination user equipment from IP addresses.
//
// A packet whose UE is attached with neither of its stacks is dropped here. Which stack
// carries a flow follows from the UE's attachment: outside dual connectivity a UE is
// attached with at most one stack, and under dual connectivity the two stacks are the
// legs of one bearer, so the choice is made per PDU, further down, by the bearer's leg
// splitter (see ~DcPdcpLegSplitter).
//
simple Ip2Nic like INetworkLayer2CellularNic
{
    parameters:
        string nodeType;
        string binderModule = default("binder");
        string bearerConfiguratorModule = default("bearerConfigurator");
        bool isNr = default(false);   // True for the NR leg (set by NrNicUe/NrNicEnb, or in the ini for a DC master eNB)
        bool hasSdap = default(false);  // When true, SDAP handles DRB/RLC assignment; Ip2Nic skips traffic classification
        bool establishBearersOnDemand = default(true);  // When false, a packet that finds no established bearer raises an error instead of establishing one (static bearers are established up front; see the staticDrbs parameter of ~BearerConfigurator)
        @display("i=block/layer");
    gates:
        // Connection to network layer
        input upperLayerIn;
        output upperLayerOut;

        // Connection to the cellular stack
        input stackIn;
        output stackOut;
}
