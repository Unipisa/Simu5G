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


package simu5g.stack.phy.feedback;

//
// This is the Lte Downlink Feedback Generator.
// It manages periodic and aperiodic channel sensing and
// feedback transmission.
//
simple LteDlFeedbackGenerator
{
    parameters:
        @display("i=block/cogwheel");
        bool isNr = default(false);            // true for the NR leg (set by NrNicUe on the nrDlFbGen submodule)

        // if true, compute feedback upon reception of CSI Reference Signal frames from the serving base station
        // (by default, the base station DOES NOT send CSI RS frames) and send it into a Feedback Packet. If false,
        // send an empty Feedback Packet to the serving base station, to make it compute feedback at the base station side
        bool computeCsiLocally = default(false);

        // target BLock Error Rate, used when CSI is computed locally
        double targetBler = default(0.001);

        // can be ALLBANDS, PREFERRED, WIDEBAND
        string feedbackType @enum(ALLBANDS,PREFERRED,WIDEBAND) = default("ALLBANDS");

        // resource allocation type ("distributed" or "localized")
        string rbAllocationType @enum(distributed,localized) = default("localized");

        // period for Periodic feedback in TTI
        int fbPeriod @unit(tti) = default(6tti);

        // time interval between sensing and transmission in TTI
        int fbDelay @unit(tti) = default(1tti);

        // true if we want to use also periodic feedback
        bool usePeriodic = default(true);

        // initial txMode (see LteCommon.h)
        string initialTxMode @enum(SINGLE_ANTENNA_PORT0,TRANSMIT_DIVERSITY) = default("SINGLE_ANTENNA_PORT0");


        string binderModule = default("binder");
        string phyModule = default("^.phy");
}
