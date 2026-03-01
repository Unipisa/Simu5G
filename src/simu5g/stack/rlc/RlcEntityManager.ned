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
// RLC entity lifecycle manager.
// Delegates entity creation/lookup to ~RlcUpperMux (TX) and ~RlcLowerMux (RX).
// Manages per-UE throughput statistics and provides deleteQueues() for handover.
//
simple RlcEntityManager
{
    parameters:
        @class("RlcEntityManager");

        @display("i=block/wheelbarrow");

        bool hasD2DSupport = default(false);
        string txEntityModuleType = default("simu5g.stack.rlc.um.UmTxEntity");
        string rxEntityModuleType = default("simu5g.stack.rlc.um.UmRxEntity");
        string tmTxEntityModuleType = default("simu5g.stack.rlc.tm.TmTxEntity");
        string tmRxEntityModuleType = default("simu5g.stack.rlc.tm.TmRxEntity");

        //# Rlc Queue
        int queueSize @unit(B) = default(2MiB);              // RLC TX entity SDU queue size (0: unlimited)
        bool mapAllLcidsToSingleBearer = default(false);     // if true, all LCIDs are mapped to a single bearer
        string nodeType @enum(INTERNET,ENODEB,GNODEB,UE);

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

        //# D2D statistics (only relevant when hasD2DSupport is true)
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

}

