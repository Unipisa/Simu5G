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


package simu5g.stack.phy.feedback.testFeedback;

import simu5g.stack.phy.feedback.LteDlFeedbackGenerator;


network TestFeedback
{
    submodules:
        lteDlFbGenerator: LteDlFeedbackGenerator {
            @display("p=96,71");
        }
        feedbackTester: FeedbackTester {
            @display("p=237,71");
        }
}
