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
// This module extends LTE MAC functionalities at the User Equipment side to support device-to-device (D2D)
// communications. Specifically, it customizes ~LteMacUe functionalities to support the transmission and reception
// of MAC Protocol Data Units (PDUs), Channel Status Information (CSI), and Buffer Status Reports (BSRs) over a
// D2D link. It supports D2D multicast transmission to nearby UEs belonging to a given multicast group.
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

