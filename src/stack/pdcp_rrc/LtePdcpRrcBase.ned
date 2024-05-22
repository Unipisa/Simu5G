
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
// Base module for the PDCP/RRC layer of LTE Stack.
//
// The LtePdcpRrcBase module serves as the base module for the PDCP/RRC layer
// in the LTE stack. It handles critical PDCP functionalities such as header 
// compression, packet flow management, and connection setup between the UE/eNB
// and the lower RLC layer.
//
// ALTERNATIVE:
//
// This module handles the Packet Data Convergence Protocol (PDCP) and 
// Radio Resource Control (RRC) layers, key components in the LTE stack.
// It defines parameters for header compression and 
// mapping to different Radio Link Control (RLC) modes.
//
// Parameters
// - headerCompressedSize: Size of the compressed header in bytes. Default is -1B (compression disabled).
// - conversationalRlc, streamingRlc, interactiveRlc, backgroundRlc: Enumerated types for different RLC modes.
//
// Gates
// - DataPort: Port for transmitting data packets.
// - EUTRAN_RRC_Sap: Control port for the RRC layer.
// - TM_Sap[], UM_Sap[], AM_Sap[]: Ports for different RLC modes - Transparent, Unacknowledged, and Acknowledged Mode SAPs.
// 
// Signals
// - receivedPacketFromUpperLayer: Signal for packets received from the upper layer.
// - sentPacketToUpperLayer: Signal for packets sent to the upper layer.
// - receivedPacketFromLowerLayer: Signal for packets received from the lower layer.
// - sentPacketToLowerLayer: Signal for packets sent to the lower layer.
//
// Statistics
// - receivedPacketFromUpperLayer: Records count and sum of packet bytes.
// - sentPacketToUpperLayer: Records count and sum of packet bytes.
// - receivedPacketFromLowerLayer: Records count and sum of packet bytes.
// - sentPacketToLowerLayer: Records count and sum of packet bytes.
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

