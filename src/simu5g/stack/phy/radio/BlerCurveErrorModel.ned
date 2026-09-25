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
// The error model of a cellular receiver: the packet error rate of a
// reception from the BLER curves of its CQI at the SINR of each allocated
// resource block -- one minus the product of the blocks' success
// probabilities -- reduced by harqReduction for every retransmission (HARQ
// soft combining). A reception with CQI 0, or with a block below the curves,
// is lost for certain.
//
simple BlerCurveErrorModel
{
    parameters:
        string binderModule = default("binder");
        double harqReduction = default(0.2); // Error rate reduction factor per HARQ retransmission
        @class(BlerCurveErrorModel);
        @display("i=block/control");
}
