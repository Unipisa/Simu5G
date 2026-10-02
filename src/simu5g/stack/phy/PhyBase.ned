//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


package simu5g.stack.phy;

//
// Base module for the Physical (PHY) layer of the LTE/NR protocol stack.
//
// Provides the common functionalities for PHY operations at both the
// eNodeB and User Equipment (UE) sides, such as the initialization of ChannelModel
// module instances, one for each Carrier Component (CC) supported by the node, and
// the transmission of frames over the radio. It provides parameters to configure
// the transmission power and direction of the node, as well as to enable or disable
// handover procedures.
//
simple PhyBase like IPhy
{
    parameters:
        @display("i=block/wrxtx");
        bool isNr = default(false);   // True for the NR leg (set by NrNicUe/NrNicEnb on their PHY submodule)
        int macNodeId;                // MacNodeId of the stack leg this PHY belongs to (set by the NIC)
        double ueTxPower @unit(dBm) = default(26dBm);
        double eNodeBTxPower @unit(dBm) = default(46dBm);
        double microTxPower @unit(dBm) = default(30dBm);

        string binderModule = default("binder");
        string radioMediumModule = default("radioMedium"); // The network's radio medium
        string componentCarrierModules = default("carrierAggregation.componentCarrier[0]"); // The component carriers this leg operates on, space-separated, the primary first; the radio medium's channel model for each, of this leg's RAT, evaluates its links
        string cellInfoModule = default("^.^.cellInfo"); // Optional; a base station registers its carriers there
        bool insideBuilding = default(false); // Whether the node is inside a building; its distance from the wall is drawn once, uniformly in [0, 25] m
        string radioModule = default("^.radio"); // This leg's radio; set to "^.nrRadio" on an NR leg, see ~NrNicUe

    gates:
        input upperGateIn;       // From upper layer
        output upperGateOut;    // To upper layer
        input radioIn;           // Frames received by the radio
        output radioOut;         // Frames to transmit, to the radio
}

