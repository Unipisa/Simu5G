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


package simu5g.stack.rlc;

// 
// Compound module for the RLC layer of LTE Stack.
//
module LteRlc {
    parameters:
        @display("i=block/transport");      
        string LteRlcUmType = default("LteRlcUm");                   // One of: "LteRlcUm", "LteRlcUmD2D"      
        bool d2dCapable;                                             // inherit the value from the parent module
        string umType = d2dCapable ? "LteRlcUmD2D" : LteRlcUmType;

    gates:
        //# 
        //# Gates connecting PDCP/RRC and RLC Layers
        //# 

        inout TM_Sap;    // Transparent Mode SAP
        inout UM_Sap;    // Unacknowledged Mode SAP
        inout AM_Sap;    // Acknowledged Mode SAP

        //# 
        //# Gates connecting RLC and MAC Layers
        //#

        input MAC_to_RLC;    // MAC to RLC
        output RLC_to_MAC;    // RLC to MAC

    submodules:
        // TM Module
        tm: LteRlcTm {
            @display("p=100,100;");
        }
        
        // UM Module        
        um: <umType> like ILteRlcUm {
            @display("p=200,100;");
        }
        
        // AM Module
        am: LteRlcAm {
            @display("p=300,100;");
        }
        
           // Muxer Module
        mux: LteRlcMux {
            @display("p=200,200");
        }

    connections:
        //#
        //# Connections from RLC (up)
        //# to TM/UM/AM modules
        //#
        tm.TM_Sap_up <--> TM_Sap;
        um.UM_Sap_up <--> UM_Sap;
        am.AM_Sap_up <--> AM_Sap;

        //#
        //# Connections from TM/UM/AM 
        //# modules to the RLC Mux
        //#
        tm.TM_Sap_down <--> mux.TM_Sap;
        um.UM_Sap_down <--> mux.UM_Sap;
        am.AM_Sap_down <--> mux.AM_Sap;

        //#
        //# Connections between Mux
        //# and RLC (down)
        //#
        mux.MAC_to_RLC <-- MAC_to_RLC;
        mux.RLC_to_MAC --> RLC_to_MAC;
}

// 
// TM Module for the RLC layer of LTE Stack.
//
simple LteRlcTm {
    parameters:
        @display("i=block/wheelbarrow");
        int queueSize @unit(B) = default(2MiB);              // RLC SDU queue size (0: unlimited)
        
        //# SDU-level statistics
        @signal[rlcPacketLossUl];
        @statistic[rlcPacketLossUl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossUl"; record=mean,sum,vector);
        @signal[rlcPacketLossDl];
        @statistic[rlcPacketLossDl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossDl"; record=mean,sum,vector);
        
        //# PDU-level statistics
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
        //# Gates connecting RLC and TM
        //# 
        inout TM_Sap_up;    // Gate to PDCP
        inout TM_Sap_down;    // Gate to RLC Mux
}

// 
// UM Module for the RLC layer of LTE Stack.
//
moduleinterface ILteRlcUm {
    parameters:
        int queueSize @unit(B);
        bool mapAllLcidsToSingleBearer;
    gates:
        inout UM_Sap_up;    // Gate to PDCP
        inout UM_Sap_down;    // Gate to RLC Mux
}

simple LteRlcUm like ILteRlcUm {
    parameters:
        @class("LteRlcUm");
        
        @display("i=block/wheelbarrow");
        
        //# Rlc Queue
        int queueSize @unit(B) = default(2MiB);              // RLC TX entity SDU queue size (0: unlimited)
        bool mapAllLcidsToSingleBearer = default(false);     // if true, all LCIDs are mapped to a single bearer
        
        //# SDU-level statistics
        @signal[rlcDelayDl];
        @statistic[rlcDelayDl](title="Delay at the rlc layer UL"; unit="s"; source="rlcDelayDl"; record=mean,vector);
        @signal[rlcThroughputDl];
        @statistic[rlcThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcThroughputDl"; record=mean,vector);
        @signal[rlcDelayUl];
        @statistic[rlcDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcDelayUl"; record=mean,vector);
        @signal[rlcThroughputUl];
        @statistic[rlcThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcThroughputUl"; record=mean,vector);
        @signal[rlcPacketLossUl];
        @statistic[rlcPacketLossUl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossUl"; record=mean,sum,vector);
        @signal[rlcPacketLossDl];
        @statistic[rlcPacketLossDl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossDl"; record=mean,sum,vector);
        @signal[rlcPacketLossTotal];
        @statistic[rlcPacketLossTotal](title="rlc Packet Loss"; unit=""; source="rlcPacketLossTotal"; record=mean,vector);

        @signal[rlcCellThroughputUl];
        @statistic[rlcCellThroughputUl](title="Cell Throughput at the rlc layer UL"; unit="Bps"; source="rlcCellThroughputUl"; record=mean);
        @signal[rlcCellThroughputDl];
        @statistic[rlcCellThroughputDl](title="Cell Throughput at the rlc layer DL"; unit="Bps"; source="rlcCellThroughputDl"; record=mean);
        @signal[rlcCellPacketLossDl];
        @statistic[rlcCellPacketLossDl](title="rlc Cell Packet Loss"; unit=""; source="rlcCellPacketLossDl"; record=mean);
        @signal[rlcCellPacketLossUl];
        @statistic[rlcCellPacketLossUl](title="rlc Cell Packet Loss"; unit=""; source="rlcCellPacketLossUl"; record=mean); 
        
        //# PDU-level statistics                 
        @signal[rlcPduDelayDl];
        @statistic[rlcPduDelayDl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayDl"; record=mean,vector);
        @signal[rlcPduThroughputDl];
        @statistic[rlcPduThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcPduThroughputDl"; record=mean,vector);
        @signal[rlcPduDelayUl];
        @statistic[rlcPduDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayUl"; record=mean,vector);
        @signal[rlcPduThroughputUl];
        @statistic[rlcPduThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcPduThroughputUl"; record=mean,vector);
        @signal[rlcPduPacketLossUl];
        @statistic[rlcPduPacketLossUl](title="rlc Packet Loss"; unit=""; source="rlcPduPacketLossUl"; record=mean,vector);
        @signal[rlcPduPacketLossDl];
        @statistic[rlcPduPacketLossDl](title="rlc Packet Loss"; unit=""; source="rlcPduPacketLossDl"; record=mean,vector);
        
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
        //# Gates connecting RLC and UM
        //# 
        inout UM_Sap_up;    // Gate to PDCP
        inout UM_Sap_down;    // Gate to RLC Mux
}

// 
// UM Module for the RLC layer of LTE Stack (with D2D).
//
simple LteRlcUmD2D extends LteRlcUm
{
    parameters:
        @class("LteRlcUmD2D");
        @display("i=block/wheelbarrow");
        
        //# SDU-level statistics
        @signal[rlcDelayD2D];
        @statistic[rlcDelayD2D](title="Delay at the rlc layer D2D"; unit="s"; source="rlcDelayD2D"; record=mean,vector);
        @signal[rlcThroughputD2D];
        @statistic[rlcThroughputD2D](title="Throughput at the rlc layer D2D"; unit="Bps"; source="rlcThroughputD2D"; record=mean,vector);
        @signal[rlcPacketLossD2D];
        @statistic[rlcPacketLossD2D](title="rlc Packet Loss"; unit=""; source="rlcPacketLossD2D"; record=mean,vector);        
      
        @signal[rlcCellPacketLossD2D];
        @statistic[rlcCellPacketLossD2D](title="rlc Cell Packet Loss"; unit=""; source="rlcCellPacketLossD2D"; record=mean,vector);
        @signal[rlcCellThroughputD2D];
        @statistic[rlcCellThroughputD2D](title="Cell Throughput at the rlc layer D2D"; unit="Bps"; source="rlcCellThroughputD2D"; record=mean,vector);     
      
        //# PDU-level statistics
        @signal[rlcPduDelayD2D];
        @statistic[rlcPduDelayD2D](title="Delay at the rlc layer D2D"; unit="s"; source="rlcPduDelayD2D"; record=mean,vector);
        @signal[rlcPduThroughputD2D];
        @statistic[rlcPduThroughputD2D](title="Throughput at the rlc layer D2D"; unit="Bps"; source="rlcPduThroughputD2D"; record=mean,vector);
        @signal[rlcPduPacketLossD2D];
        @statistic[rlcPduPacketLossD2D](title="rlc Packet Loss"; unit=""; source="rlcPduPacketLossD2D"; record=mean,vector);
}

// 
// AM Module for the RLC layer of LTE Stack.
//
simple LteRlcAm {
    parameters:
        @display("i=block/wheelbarrow");
        
        @signal[rlcDelayDl];
        @statistic[rlcDelayDl](title="Delay at the rlc layer UL"; unit="s"; source="rlcDelayDl"; record=mean);
        @signal[rlcThroughputDl];
        @statistic[rlcThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcThroughputDl"; record=mean);
        @signal[rlcDelayUl];
        @statistic[rlcDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcDelayUl"; record=mean);
        @signal[rlcThroughputUl];
        @statistic[rlcThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcThroughputUl"; record=mean);
        @signal[rlcPduDelayDl];
        @statistic[rlcPduDelayDl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayDl"; record=mean);
        @signal[rlcPduThroughputDl];
        @statistic[rlcPduThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcPduThroughputDl"; record=mean);
        @signal[rlcPduDelayUl];
        @statistic[rlcPduDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayUl"; record=mean);
        @signal[rlcPduThroughputUl];
        @statistic[rlcPduThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcPduThroughputUl"; record=mean);
        @signal[rlcCellThroughputUl];
        @statistic[rlcCellThroughputUl](title="Cell Throughput at the rlc layer UL"; unit="Bps"; source="rlcCellThroughputUl"; record=mean);
        @signal[rlcCellThroughputDl];
        @statistic[rlcCellThroughputDl](title="Cell Throughput at the rlc layer DL"; unit="Bps"; source="rlcCellThroughputDl"; record=mean);
        @signal[rlcCellPacketLossDl];
        @statistic[rlcCellPacketLossDl](title="rlc Cell Packet Loss"; unit=""; source="rlcCellPacketLossDl"; record=mean);
        @signal[rlcCellPacketLossUl];
        @statistic[rlcCellPacketLossUl](title="rlc Cell Packet Loss"; unit=""; source="rlcCellPacketLossUl"; record=mean);
        @signal[rlcPacketLossUl];
        @statistic[rlcPacketLossUl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossUl"; record=mean);
        @signal[rlcPacketLossDl];
        @statistic[rlcPacketLossDl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossDl"; record=mean);
        @signal[rlcPacketLossTotal];
        @statistic[rlcPacketLossTotal](title="rlc Packet Loss"; unit=""; source="rlcPacketLossTotal"; record=mean);
        @signal[rlcPduPacketLossUl];
        @statistic[rlcPduPacketLossUl](title="rlc Packet Loss"; unit=""; source="rlcPduPacketLossUl"; record=mean);
        @signal[rlcPduPacketLossDl];
        @statistic[rlcPduPacketLossDl](title="rlc Packet Loss"; unit=""; source="rlcPduPacketLossDl"; record=mean);
        
    gates:
        //# 
        //# Gates connecting RLC and AM
        //# 
        inout AM_Sap_up;    // Gate to PDCP
        inout AM_Sap_down;    // Gate to RLC Mux
}

// 
// Muxer for the RLC layer of LTE Stack.
// Mux/Demux of data to/from MAC layer
//
simple LteRlcMux {
    parameters:
        @display("i=block/classifier");                        // Display preferences

    gates:
        //# 
        //# Gates connecting TM/UM/AM modules and RLC Mux
        //# 
        inout TM_Sap;    // Transparent Mode SAP
        inout UM_Sap;    // Unacknowledged Mode SAP
        inout AM_Sap;    // Acknowledged Mode SAP

        //# 
        //# Gates connecting RLC and MAC Layers
        //#
        input MAC_to_RLC;    // MAC to RLC
        output RLC_to_MAC;    // RLC to MAC
}

// 
// Transmit Buffer of RLC AM
//
simple AmTxQueue {
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int fragmentSize @unit(B) = default(30B);        // Size of fragments
        int maxRtx = default(3);
        double pduRtxTimeout @unit(s) = default(2.0s);
        double ctrlPduRtxTimeout @unit(s) = default (2.0s);
        double bufferStatusTimeout @unit(s) = default (2.0s);
        int txWindowSize = default (200);
}

// 
// Receive Buffer of RLC AM
//
simple AmRxQueue {
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int rxWindowSize = default(200);
        double ackReportInterval @unit(s) = 0.10s;
        double statusReportInterval @unit(s) = 0.20s;
        double timeout @unit(s) = default(1s);            // Timeout for RX Buffer
}

//
// Entities for the RLC module
//

// 
// Transmit Entity of RLC UM
//
simple UmTxEntity {
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int fragmentSize @unit(B) = default(30B);        // Size of fragments
}

// 
// Receive Entity of RLC UM
//
simple UmRxEntity {
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        double timeout @unit(s) = default(1s);            // Timeout for RX Buffer
        int rxWindowSize = default(16); 
}

// 
// Transmit Entity of RLC AM
//
simple AmTxEntity {
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int fragmentSize @unit(B) = default(30B);        // Size of fragments
        int maxRtx = default(3);
        double pduRtxTimeout @unit(s) = default(2.0s);
        double ctrlPduRtxTimeout @unit(s) = default (2.0s);
        double bufferStatusTimeout @unit(s) = default (2.0s);
        int txWindowSize = default (16);
}

// 
// Receive Entity of RLC AM
//
simple AmRxEntity {
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int rxWindowSize = default(20);
        double ackReportInterval @unit(s) = 0.10s;
        double statusReportInterval @unit(s) = 0.20s;
        double timeout @unit(s) = default(1s);            // Timeout for RX Buffer
}
