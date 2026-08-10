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

simple NtnPhyBase
{
    parameters:
        @display("i=block/wrxtx;bgb=704,443;bgl=3");
        string binderModule = default("binder");
        string channelModelModule = default("^.channelModel[0]");
        string antennaModelModule = default("^.antennaModel");
        string linkType @enum("feeder","service");
        double feederLinkFrequencyOffset @unit(GHz) = default(25GHz);

        // Adds the geometric propagation delay of each transparent-NTN radio hop to the
        // frames this PHY sends. Set to false to restore the instantaneous-hop behaviour of
        // earlier revisions, which is the only way to isolate the effect of the delay from
        // an unrelated change.
        bool useGeometricPropagationDelay = default(true);

        // Model the transparent payload as an analog bent-pipe repeater, which does not
        // re-buffer the frame: its transmission time is then charged once end to end, on the
        // hop where the satellite transmits, instead of once per radio hop. Set to false to
        // keep charging the frame duration on every hop.
        bool transparentPayloadRelay = default(true);

        // Largest propagation delay accepted on a single radio hop. A longer hop aborts the
        // run rather than producing an implausible latency. 0s disables the check. The
        // default covers a GEO link down to the horizon (about 41700km, i.e. 139ms).
        double maxHopPropagationDelay @unit(s) = default(150ms);

        // Re-report a hop's geometry once its one-way delay has moved by more than this since
        // the last report. Keeps a long LEO run readable while still showing the geometry
        // evolving; with a fixed GEO satellite each hop is reported exactly once. 0s reports
        // every evaluation and is only useful for debugging.
        double propagationDelayReportThreshold @unit(s) = default(0.1ms);

        @signal[ntnHopPropagationDelay];
        @statistic[ntnHopPropagationDelay](title="Propagation delay of a transparent NTN radio hop"; unit="s"; source="ntnHopPropagationDelay"; record=mean,min,max,vector);
        @signal[ntnHopSlantRange];
        @statistic[ntnHopSlantRange](title="Slant range of a transparent NTN radio hop"; unit="m"; source="ntnHopSlantRange"; record=mean,min,max,vector);
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @loose @directIn;
}
