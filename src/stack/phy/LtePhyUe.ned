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


package simu5g.stack.phy;

//
// User Equipment LtePhy module of PHY Layer
//
simple LtePhyUe extends LtePhyBase
{
    parameters:
        @class("LtePhyUe");

        string rlcUmModule = default("^.rlc.um"); // or "^.nrRlc.um"
        string pdcpRrcModule = default("^.pdcpRrc");
        string ip2nicModule = default("^.ip2nic");
        string feedbackGeneratorModule = default("^.dlFbGen"); // TODO or "^.nrDlFbGen"

        // if true, get minRssi from PhyPisaData
        bool minRssiDefault = default(true);

        // configurable minimum threshold RSSI for attaching to an eNB (meaningful only if minRssiDefault==false)
        double minRssi @unit("dB") = default(-99.0dB);

        bool hasCollector = default(false); // true when node has a collector module (ueCollector/NRueCollector) for this PHY module

        @signal[distance];
        @statistic[distance](title="distance between UE and serving base station"; unit="meters"; source="distance"; record=mean,vector);

        @signal[servingCell];
        @statistic[servingCell](title="ID of the serving cell for the UE"; unit=""; source="servingCell"; record=vector);

        //# CQI statistics
        @signal[averageCqiDl];
        @statistic[averageCqiDl](title="Average Cqi reported in DL"; unit="cqi"; source="averageCqiDl"; record=mean,count,vector);
        @signal[averageCqiUl];
        @statistic[averageCqiUl](title="Average Cqi reported in UL"; unit="cqi"; source="averageCqiUl"; record=mean,count,vector);
}

