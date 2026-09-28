//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.phy.radio;

//
// The receiver of a cellular radio: noise figure and cable loss, the
// reception decision and its random draw, and the SINR statistics; the error
// model computes the packet error rate the decision is drawn against. The
// decision is one uniform draw from this module's RNG.
//
// The SINR statistics describe the UE end of a link, in both directions: a
// UE's receiver records the SINR of its downlink receptions and also that of
// its own uplink transmissions, as measured at the base station, so the
// uplink figures stay per UE. A base station's receiver records none of
// them. On a radio that serves more than one carrier, each statistic is also
// recorded per carrier, under the statistic's name followed by the carrier
// frequency in MHz (measuredSinrDl-3500MHz, for example).
//
module CellularReceiver
{
    parameters:
        double noiseFigure @unit(dB) = default(7dB);
        double cableLoss @unit(dB) = default(2dB);   // Loss between the antenna and the receiver
        @class(CellularReceiver);
        @display("i=block/rx");
        @signal[rcvdSinrDl];
        @statistic[rcvdSinrDl](title="SINR measured at packet reception, DL"; unit="dB"; source="rcvdSinrDl"; record=mean,vector);
        @signal[rcvdSinrUl];
        @statistic[rcvdSinrUl](title="SINR measured at packet reception, UL"; unit="dB"; source="rcvdSinrUl"; record=mean,vector);
        @signal[measuredSinrDl];
        @statistic[measuredSinrDl](title="SINR measured at feedback computation, DL"; unit="dB"; source="measuredSinrDl"; record=mean,vector);
        @signal[measuredSinrUl];
        @statistic[measuredSinrUl](title="SINR measured at feedback computation, UL"; unit="dB"; source="measuredSinrUl"; record=mean,vector);
        @signal[rcvdSinrDl-*];
        @statisticTemplate[rcvdSinrDl](title="SINR measured at packet reception on one carrier, DL"; unit="dB"; record=mean,vector);
        @signal[rcvdSinrUl-*];
        @statisticTemplate[rcvdSinrUl](title="SINR measured at packet reception on one carrier, UL"; unit="dB"; record=mean,vector);
        @signal[measuredSinrDl-*];
        @statisticTemplate[measuredSinrDl](title="SINR measured at feedback computation on one carrier, DL"; unit="dB"; record=mean,vector);
        @signal[measuredSinrUl-*];
        @statisticTemplate[measuredSinrUl](title="SINR measured at feedback computation on one carrier, UL"; unit="dB"; record=mean,vector);
    submodules:
        errorModel: BlerCurveErrorModel {
            @display("p=100,100");
        }
}
