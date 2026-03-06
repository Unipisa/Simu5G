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

package simu5g.stack.handoverX2Forwarder;

//
// Manages the exchange of messages during the handover process.
// It handles the signaling and coordination required to
// transfer both control and data-plane information from the source eNodeB to the
// target eNodeB. The module directly interfaces with the ~LteX2Manager to
// facilitate this inter-eNodeB communication.
//
simple HandoverX2Forwarder like IHandoverX2Forwarder
{
    parameters:
        @display("i=block/cogwheel");
        @class("HandoverX2Forwarder");

        bool losslessHandover = default(false);
        string handoverPacketHolderModule = default("^.handoverPacketHolder");

    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;

        output tunnelOut; // data packets received via the X2 tunnel during handover
        input dataIn; // data packets from HandoverPacketHolder to forward via X2
}
