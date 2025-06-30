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


package simu5g.stack.mac;

//
// Base module for the Medium Access Control (MAC) layer of the LTE protocol stack.
//
// This module provides common functionalities for MAC operations at both the
// eNodeB and User Equipment (UE) sides, such as functions to exchange MAC Service
// Data Units (SDUs) with the Radio Link Control (RLC) layer, and MAC Protocol Data
// Units (PDUs) with the physical layer.
//
simple LteMacBase like ILteMac
{
    parameters:
        @display("i=block/mac");
        string binderModule = default("binder");
        string packetFlowManagerModule = default("^.packetFlowManager"); // TODO or nrPacketFlowManager

        //# Mac Queues
        int queueSize @unit(B) = default(2MiB);              // MAC Buffers queue size

        //# Mac MIB
        bool muMimo = default(true);

        //# H-ARQ
        int harqProcesses = default(8);
        int maxHarqRtx = default(3);
        int harqFbEvaluationTimer = default(4);              // number of slots for sending back HARQ FB

        //# Statistics display (in GUI)
        bool statDisplay = default(false);

        //#
        //# Statistics recording
        //#
        @signal[macDelayDl];
        @statistic[macDelayDl](title="Delay at the MAC layer UL"; unit="s"; source="macDelayDl"; record=mean,vector);
        @signal[macThroughputDl];
        @statistic[macThroughputDl](title="Throughput at the MAC layer DL"; unit="Bps"; source="macThroughputDl"; record=mean);
        @signal[macDelayUl];
        @statistic[macDelayUl](title="Delay at the MAC layer UL"; unit="s"; source="macDelayUl"; record=mean,vector);
        @signal[macThroughputUl];
        @statistic[macThroughputUl](title="Throughput at the MAC layer UL"; unit="Bps"; source="macThroughputUl"; record=mean);
        @signal[macCellThroughputUl];
        @statistic[macCellThroughputUl](title="Cell Throughput at the MAC layer UL"; unit="Bps"; source="macCellThroughputUl"; record=mean);
        @signal[macCellThroughputDl];
        @statistic[macCellThroughputDl](title="Cell Throughput at the MAC layer DL"; unit="Bps"; source="macCellThroughputDl"; record=mean);
        @signal[macCellPacketLossDl];
        @statistic[macCellPacketLossDl](title="Mac Cell Packet Loss Dl"; unit=""; source="macCellPacketLossDl"; record=mean);
        @signal[macCellPacketLossUl];
        @statistic[macCellPacketLossUl](title="Mac Cell Packet Loss Ul"; unit=""; source="macCellPacketLossUl"; record=mean);
        @signal[macPacketLossUl];
        @statistic[macPacketLossUl](title="Mac Packet Loss Ul"; unit=""; source="macPacketLossUl"; record=mean);
        @signal[macPacketLossDl];
        @statistic[macPacketLossDl](title="Mac Packet Loss Dl"; unit=""; source="macPacketLossDl"; record=mean);
        @signal[macBufferOverFlowDl];
        @statistic[macBufferOverFlowDl](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowDl"; record=mean);
        @signal[macBufferOverFlowUl];
        @statistic[macBufferOverFlowUl](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowUl"; record=mean);
        @signal[macBufferOverFlowD2D];
        @statistic[macBufferOverFlowD2D](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowD2D"; record=mean);
        @signal[harqErrorRateUl];
        @statistic[harqErrorRateUl](title="Harq Error Rate Ul"; unit=""; source="harqErrorRateUl"; record=mean,vector);
        @signal[harqErrorRateDl];
        @statistic[harqErrorRateDl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRateDl"; record=mean,vector);
        @signal[harqTxAttemptsDl];
        @statistic[harqTxAttemptsDl](title="Harq Tx Attempts Dl"; unit=""; source="harqTxAttemptsDl"; record=mean,vector);
        @signal[harqTxAttemptsUl];
        @statistic[harqTxAttemptsUl](title="Harq Tx Attempts Ul"; unit=""; source="harqTxAttemptsUl"; record=mean,vector);
        @signal[harqErrorRate_1st_Ul];
        @statistic[harqErrorRate_1st_Ul](title="Harq Error Rate Ul (1st tx)"; unit=""; source="harqErrorRate_1st_Ul"; record=mean,vector);
        @signal[harqErrorRate_1st_Dl];
        @statistic[harqErrorRate_1st_Dl](title="Harq Error Rate Dl (1st tx)"; unit=""; source="harqErrorRate_1st_Dl"; record=mean,vector);
        @signal[harqErrorRate_2nd_Ul];
        @statistic[harqErrorRate_2nd_Ul](title="Harq Error Rate Ul (2nd tx)"; unit=""; source="harqErrorRate_2nd_Ul"; record=mean,vector);
        @signal[harqErrorRate_2nd_Dl];
        @statistic[harqErrorRate_2nd_Dl](title="Harq Error Rate Dl (2nd tx)"; unit=""; source="harqErrorRate_2nd_Dl"; record=mean,vector);
        @signal[harqErrorRate_3rd_Ul];
        @statistic[harqErrorRate_3rd_Ul](title="Harq Error Rate Ul (3rd tx)"; unit=""; source="harqErrorRate_3rd_Ul"; record=mean,vector);
        @signal[harqErrorRate_3rd_Dl];
        @statistic[harqErrorRate_3rd_Dl](title="Harq Error Rate Dl (3rd tx)"; unit=""; source="harqErrorRate_3rd_Dl"; record=mean,vector);
        @signal[harqErrorRate_4th_Ul];
        @statistic[harqErrorRate_4th_Ul](title="Harq Error Rate Ul (4th tx)"; unit=""; source="harqErrorRate_4th_Ul"; record=mean,vector);
        @signal[harqErrorRate_4th_Dl];
        @statistic[harqErrorRate_4th_Dl](title="Harq Error Rate Dl (4th tx)"; unit=""; source="harqErrorRate_4th_Dl"; record=mean,vector);
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
        //# Gates connecting RLC and MAC Layers
        //#

        input RLC_to_MAC;    // RLC to MAC
        output MAC_to_RLC;    // MAC to RLC

        //#
        //# Gates connecting MAC and PHY Layers
        //#

        input PHY_to_MAC;    // PHY to MAC
        output MAC_to_PHY;    // MAC to PHY
}

