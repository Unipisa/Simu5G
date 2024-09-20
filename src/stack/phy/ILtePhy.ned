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
// Interface for the Physical (PHY) layer of the LTE stack.
//
// The LtePhy module interface provides the definition for the PHY layer in the LTE protocol stack.
// The PHY layer sends and receives MAC Protocol Data Units (SDUs) from the Medium Access Control (MAC)
// layer and transmits and receives frames over the radio medium. The PHY layer is responsible for handling
// the transmission and reception of radio frames, including measuring the Signal to
// Interference-plus-Noise Ratio (SINR) of received frames and determining whether they were received
// correctly or not. This is accomplished by utilizing an abstraction of the channel model without modeling
// the transmission and reception of individual symbols. Specifically, on the transmitting side, radio frames
// are sent directly to the intended receiver (via the sendDirect() method). On the receiving side, the
// ChannelModel class is used to obtain the SINR resulting from channel effects (such as path loss,
// fading/shadowing), as well as inter-cell interference, and to check the correctness of the reception using
// Block Error Rate (BLER) curves.
// The PHY layer also measures the uplink and downlink channel quality by computing Channel Status
// Information (CSI) for User Equipments (UEs) in both uplink and downlink directions.
// Handover management is also the responsibility of the PHY layer.
//
moduleinterface ILtePhy
{
    parameters:
        @display("i=block/wrxtx");

    gates:
        input upperGateIn;
        output upperGateOut;
        input radioIn;
}

