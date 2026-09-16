//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.d2d.phy.channelmodel;

import simu5g.stack.phy.channelmodel.StochasticChannelModel;

//
// D2D-capable channel model: ~StochasticChannelModel plus the device-to-device
// channel math (RSRP/SINR, D2D interference, D2D reception decision). It is the
// channel model of the D2D NICs on all propagation studies; the pathLossType
// parameter (inherited from ~StochasticChannelModel) selects TR 36.814, 36.873
// or 38.901.
//
simple D2dChannelModel extends StochasticChannelModel
{
    parameters:
        @class("D2dChannelModel");

        // Whether interference from other D2D transmissions is included in the D2D SINR
        bool d2dInterference = default(true);

        // Statistics
        @signal[rcvdSinrD2D];
        @statistic[rcvdSinrD2D](title="SINR measured at packet reception, D2D"; unit="dB"; source="rcvdSinrD2D"; record=mean,vector);
}
