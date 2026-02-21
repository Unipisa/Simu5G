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


package simu5g.stack.pdcp;

//
// The LtePdcp module implements the functionalities of the Packet Data
// Convergence Protocol (PDCP). It enables header compression and maps incoming IP packets
// to different Radio Link Control (RLC) modes. This module dynamically creates
// ~LteTxPdcpEntity and ~LteRxPdcpEntity submodules to handle individual transmission/reception connections
//
simple LtePdcp like ILtePdcp
{
    parameters:
        @display("i=block/fork");
        string binderModule = default("binder");
        string packetFlowObserverModule = default("^.packetFlowObserver"); // optional
        string nrPacketFlowObserverModule = default("^.nrPacketFlowObserver"); // optional
        int headerCompressedSize @unit(B) = default(-1B);    // Header compressed size (bytes) ( -1B = compression disabled
        string rxEntityModuleType = default("simu5g.stack.pdcp.LteRxPdcpEntity");
        string txEntityModuleType = default("simu5g.stack.pdcp.LteTxPdcpEntity");
        bool isNR = default(false);           // true for NR PDCP variants (NrPdcpEnb, NrPdcpUe)
        bool hasD2DSupport = default(false);  // true for D2D-capable variants

        //# Statistics
        @signal[receivedPacketFromUpperLayer];
        @statistic[receivedPacketFromUpperLayer](source="receivedPacketFromUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[receivedPacketFromLowerLayer];
        @statistic[receivedPacketFromLowerLayer](source="receivedPacketFromLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToUpperLayer];
        @statistic[sentPacketToUpperLayer](source="sentPacketToUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToLowerLayer];
        @statistic[sentPacketToLowerLayer](source="sentPacketToLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
    gates:
        //#
        //# Gates connecting UE/eNB and PDCP/RRC Layer
        //#
        //# Data Ports

        input upperLayerIn;        // Port to receive data packets from upper layer
        output upperLayerOut;      // Port to send data packets to upper layer

        //#
        //# Gates connecting PDCP/RRC and RLC Layer
        //#
        //# Data Ports

        input rlcIn;               // from RLC layer
        output rlcOut;             // to RLC layer
}

