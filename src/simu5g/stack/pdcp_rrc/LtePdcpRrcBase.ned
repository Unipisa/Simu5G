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


package simu5g.stack.pdcp_rrc;

//
// The LtePdcpRrcBase module implements the functionalities of the Packet Data
// Convergence Protocol (PDCP). It enables header compression and maps incoming IP packets
// to different Radio Link Control (RLC) modes. This module dynamically creates
// ~LteTxPdcpEntity and ~LteRxPdcpEntity submodules to handle individual transmission/reception connections
//
simple LtePdcpRrcBase like ILtePdcpRrc
{
    parameters:
        @display("i=block/fork");
        string binderModule = default("binder");
        string packetFlowManagerModule = default("^.packetFlowManager"); // optional
        string nrPacketFlowManagerModule = default("^.nrPacketFlowManager"); // optional
        int headerCompressedSize @unit(B) = default(-1B);    // Header compressed size (bytes) ( -1B = compression disabled
        string conversationalRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default("UM");
        string streamingRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default("UM");
        string interactiveRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default("UM");
        string backgroundRlc @enum(TM,UM,AM,UNKNOWN_RLC_TYPE) = default("UM");

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

        inout DataPort;            // Port to transmit data packets

        //# Control Ports

        inout EUTRAN_RRC_Sap;    // Control of RRC Layer

        //#
        //# Gates connecting PDCP/RRC and RLC Layer
        //#
        //# Data Ports

        inout TM_Sap[];            // Transparent Mode SAP
        inout UM_Sap[];            // Unacknowledged Mode SAP
        inout AM_Sap[];            // Acknowledged Mode SAP
}

