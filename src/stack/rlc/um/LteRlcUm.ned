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


package simu5g.stack.rlc.um;

//
// This module implements the functionalities of the Unacknowledged Mode (UM) of the
// Radio Link Control (RLC) layer.
// Transmitting- and receiving-side functionalities are provided by the ~UmTxEntity and
// ~UmRxEntity submodules, respectively. The latter are created dynamically whenever
// a new connection is detected. This module is responsible for identifying the flow to which
// a RLC Service Data Unit (SDU) or Protocol Data Unit (PDU) belongs and routing it to
// the corresponding transmitting or receiving entity for further processing.
//
simple LteRlcUm like ILteRlcUm
{
    parameters:
        @class("LteRlcUm");

        @display("i=block/wheelbarrow");

        //# Rlc Queue
        int queueSize @unit(B) = default(2MiB);              // RLC TX entity SDU queue size (0: unlimited)
        bool mapAllLcidsToSingleBearer = default(false);     // if true, all LCIDs are mapped to a single bearer
        string nodeType @enum(INTERNET,ENODEB,GNODEB,UE);

        //# SDU-level statistics
        @signal[rlcDelayDl];
        @statistic[rlcDelayDl](title="Delay at the rlc layer DL"; unit="s"; source="rlcDelayDl"; record=mean,vector);
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

