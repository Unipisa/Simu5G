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
// NTN-specific NR PHY for User Equipments. It sends air frames to the serving
// gNodeB through the transparent satellite path, and evaluates the reception of
// air frames arriving from that path with a dedicated NTN channel model.
//
simple NtnPhyUe extends NrPhyUe
{
    parameters:
        @class("NtnPhyUe");
        string ntnChannelModelModule = default("^.ntnChannelModel[0]");
        string ntnAntennaModelModule = default("^.ntnAntennaModel");

        // Declared with the same names and defaults as in NtnPhyBase, so that a single
        // "**." assignment configures every radio on the transparent path -- the UE, the
        // gateway feeder NIC, and both satellite NICs -- rather than three of the four.
        // See NtnPhyBase.ned for what each one means.
        bool useGeometricPropagationDelay = default(true);
        bool transparentPayloadRelay = default(true);
        double maxHopPropagationDelay @unit(s) = default(150ms);
        double propagationDelayReportThreshold @unit(s) = default(0.1ms);

        @signal[ntnHopPropagationDelay];
        @statistic[ntnHopPropagationDelay](title="Propagation delay of a transparent NTN radio hop"; unit="s"; source="ntnHopPropagationDelay"; record=mean,min,max,vector);
        @signal[ntnHopSlantRange];
        @statistic[ntnHopSlantRange](title="Slant range of a transparent NTN radio hop"; unit="m"; source="ntnHopSlantRange"; record=mean,min,max,vector);
}
