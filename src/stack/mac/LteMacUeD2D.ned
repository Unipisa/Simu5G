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
// Long-Term Evolution (LTE) Medium Access Control (MAC) layer for user equipment (UE) used for device-to-device (D2D) communication
//
// This module manages the Medium Access Control (MAC) layer functionalities in the User Equipment (UE) for 
// direct device-to-device (D2D) communication within the LTE network. It extends the basic MAC functionality of 
// LTE UE to support D2D features, enabling two UE devices to communicate directly without the need to route 
// traffic through the base station. This is crucial for enhancing network performance, reducing latency, and 
// offloading traffic from the core network, making it a vital component in the next-generation 5G ecosystem.
//
simple LteMacUeD2D extends LteMacUe
{
    parameters:
        @class("LteMacUeD2D");

        bool usePreconfiguredTxParams = default(false);
        int d2dCqi = default(7);
        string rlcUmModule = default("^.rlc.um");
        string pdcpRrcModule = default("^.pdcpRrc");

        @signal[harqErrorRate_1st_D2D];
        @statistic[harqErrorRate_1st_D2D](title="Harq Error Rate D2D (1st Tx)"; unit=""; source="harqErrorRate_1st_D2D"; record=mean,vector);
        @signal[harqErrorRate_2nd_D2D];
        @statistic[harqErrorRate_2nd_D2D](title="Harq Error Rate D2D (2nd Tx)"; unit=""; source="harqErrorRate_2nd_D2D"; record=mean,vector);
        @signal[harqErrorRate_3rd_D2D];
        @statistic[harqErrorRate_3rd_D2D](title="Harq Error Rate D2D (3rd Tx)"; unit=""; source="harqErrorRate_3rd_D2D"; record=mean,vector);
        @signal[harqErrorRate_4th_D2D];
        @statistic[harqErrorRate_4th_D2D](title="Harq Error Rate D2D (4th Tx)"; unit=""; source="harqErrorRate_4th_D2D"; record=mean,vector);
        @signal[harqErrorRateD2D];
        @statistic[harqErrorRateD2D](title="Harq Error Rate D2D"; unit=""; source="harqErrorRateD2D"; record=mean,vector);
        @signal[macPacketLossD2D];
        @statistic[macPacketLossD2D](title="Mac Packet Loss D2D"; unit=""; source="macPacketLossD2D"; record=mean);
        @signal[macDelayD2D];
        @statistic[macDelayD2D](title="Delay at the MAC layer D2D"; unit="s"; source="macDelayD2D"; record=mean,vector);
        @signal[macThroughputD2D];
        @statistic[macThroughputD2D](title="Throughput at the MAC layer D2D"; unit="Bps"; source="macThroughputD2D"; record=mean);

        @signal[rcvdD2DModeSwitchNotification];
        @statistic[rcvdD2DModeSwitchNotification](title="Reception of mode switch notification (tx side)"; unit=""; source="rcvdD2DModeSwitchNotification"; record=count,vector);
}

