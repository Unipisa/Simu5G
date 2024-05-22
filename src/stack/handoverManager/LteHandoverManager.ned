
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

package simu5g.stack.handoverManager;

//
// LteHandoverManager is a module responsible for managing the handover process in a
// Long-Term Evolution (LTE) network. It handles the signaling and coordination
// required to transfer an active session from one eNodeB to another, ensuring
// minimal disruption. The module directly interfaces with the X2 Manager to
// facilitate this inter-eNodeB communication.
//
simple LteHandoverManager like ILteHandoverManager
{
    parameters:
        @display("i=block/cogwheel");
        @class("LteHandoverManager");

        bool losslessHandover = default(false);
        string ip2nicModule = default("^.ip2nic");

    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
}
