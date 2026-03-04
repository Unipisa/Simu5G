package simu5g.stack.rlc;

//
// Lower-layer RLC packet dispatcher.
//
// Dispatches MAC packets to the correct RX entity or TX entity,
// and collects TX entity output to route to the MAC layer.
//
simple RlcMux
{
    parameters:
        @class("RlcMux");
        @display("p=100,200;i=block/dispatch");
        string entityManagerModule;

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

        @signal[rlcCellThroughputUl];
        @statistic[rlcCellThroughputUl](title="Cell Throughput at the rlc layer UL"; unit="Bps"; source="rlcCellThroughputUl"; record=simu5g_rateavg);
        @signal[rlcCellThroughputDl];
        @statistic[rlcCellThroughputDl](title="Cell Throughput at the rlc layer DL"; unit="Bps"; source="rlcCellThroughputDl"; record=simu5g_rateavg);

        //# PDU-level statistics
        @signal[rlcPduDelayDl];
        @statistic[rlcPduDelayDl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayDl"; record=mean,vector);
        @signal[rlcPduThroughputDl];
        @statistic[rlcPduThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcPduThroughputDl"; record=simu5g_rateavg,vector);
        @signal[rlcPduDelayUl];
        @statistic[rlcPduDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayUl"; record=mean,vector);
        @signal[rlcPduThroughputUl];
        @statistic[rlcPduThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcPduThroughputUl"; record=simu5g_rateavg,vector);

        //# D2D statistics
        @signal[rlcDelayD2D];
        @statistic[rlcDelayD2D](title="Delay at the rlc layer D2D"; unit="s"; source="rlcDelayD2D"; record=mean,vector);
        @signal[rlcThroughputD2D];
        @statistic[rlcThroughputD2D](title="Throughput at the rlc layer D2D"; unit="Bps"; source="rlcThroughputD2D"; record=simu5g_rateavg,vector);

        @signal[rlcCellThroughputD2D];
        @statistic[rlcCellThroughputD2D](title="Cell Throughput at the rlc layer D2D"; unit="Bps"; source="rlcCellThroughputD2D"; record=simu5g_rateavg,vector);

        @signal[rlcPduDelayD2D];
        @statistic[rlcPduDelayD2D](title="Delay at the rlc layer D2D"; unit="s"; source="rlcPduDelayD2D"; record=mean,vector);
        @signal[rlcPduThroughputD2D];
        @statistic[rlcPduThroughputD2D](title="Throughput at the rlc layer D2D"; unit="Bps"; source="rlcPduThroughputD2D"; record=simu5g_rateavg,vector);

    gates:
        input macIn;
        output macOut;
        output toRxEntity[];          // dynamic: dispatches to RX entity in gates
        input fromTxEntity[];         // dynamic: receives from TX entity out gates
        output macToTxEntity[];       // dynamic: dispatches MAC SDU requests to TX entity macIn gates
}
