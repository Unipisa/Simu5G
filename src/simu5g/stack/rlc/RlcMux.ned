//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.rlc;

//
// Lower-layer RLC packet dispatcher.
//
// Dispatches MAC packets to the correct RX entity or TX entity,
// and collects TX entity output to route to the MAC layer.
//
simple RlcMux like IRlcMux
{
    parameters:
        @class("RlcMux");
        @display("p=100,200;i=block/dispatch");
        string bearerManagementModule = default("^.rrc.bearerManagement");

        @signal[receivedPacketFromLowerLayer];
        @statistic[receivedPacketFromLowerLayer](source="receivedPacketFromLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToLowerLayer];
        @statistic[sentPacketToLowerLayer](source="sentPacketToLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);

        //# SDU-level statistics
        @signal[rlcDelayDl];
        @statistic[rlcDelayDl](title="Delay at the rlc layer DL"; unit="s"; source="rlcDelayDl"; record=mean,vector);
        @signal[rlcThroughputDl];
        @statistic[rlcThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcThroughputDl"; record=simu5g_rateavg,vector);
        @signal[rlcDelayUl];
        @statistic[rlcDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcDelayUl"; record=mean,vector);
        @signal[rlcThroughputUl];
        @statistic[rlcThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcThroughputUl"; record=simu5g_rateavg,vector);

        //# PDU-level statistics
        @signal[rlcPduDelayDl];
        @statistic[rlcPduDelayDl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayDl"; record=mean,vector);
        @signal[rlcPduThroughputDl];
        @statistic[rlcPduThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcPduThroughputDl"; record=simu5g_rateavg,vector);
        @signal[rlcPduDelayUl];
        @statistic[rlcPduDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayUl"; record=mean,vector);
        @signal[rlcPduThroughputUl];
        @statistic[rlcPduThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcPduThroughputUl"; record=simu5g_rateavg,vector);

    gates:
        input macIn;
        output macOut;
        output toRxEntity[];          // Dynamic: dispatches to RX entity in gates
        input fromTxEntity[];         // Dynamic: receives from TX entity out gates
        output macToTxEntity[];       // Dynamic: dispatches MAC SDU requests to TX entity macIn gates
}
