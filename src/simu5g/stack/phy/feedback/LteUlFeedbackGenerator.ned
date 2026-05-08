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


package simu5g.stack.phy.feedback;

//
// This is the LTE Uplink Feedback Generator.
//
simple LteUlFeedbackGenerator
{
    parameters:
        @display("i=block/cogwheel");
        double targetBler = default(0.001);
        string binderModule = default("binder");
        string phyModule = default("^.phy");

        // can be ALLBANDS, PREFERRED, WIDEBAND
        string feedbackType @enum(ALLBANDS,PREFERRED,WIDEBAND) = default("ALLBANDS");

        // resource allocation type ("distributed" or "localized")
        string rbAllocationType @enum(distributed,localized) = default("localized");

        // initial txMode (see LteCommon.h)
        string initialTxMode @enum(SINGLE_ANTENNA_PORT0,TRANSMIT_DIVERSITY) = default("SINGLE_ANTENNA_PORT0");

}
