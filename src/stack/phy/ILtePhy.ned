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
// Interface for the Physical (PHY) layer of LTE Stack.
// 
// The LtePhy module interface provides the definition for the PHY layer in the LTE protocol stack.
// The PHY layer sends/receives MAC Protocol Data Units (SDUs) from the Medium Access Control (MAC) 
// layer and sends/receives frames over the radio medium. The PHY layer is responsible to handle
// the transmission and reception of radio frames including measuring the Signal to 
// Interference-plus-Noise Ratio (SINR) of received frames and deciding whether they were received
// correctly or not. This is accomplished by using an abstraction of the channel model without modeling
// the transmission/reception of individual symbols. In particular, on the transmitting side radio frames 
// are sent directly to the intended receiver (via sendDirect() method). On the receiving side, the 
// ChannelModel class is used to obtain the SINR resulting from channel effects (such as path loss, 
// fading/shadowing), and inter-cell interference and checking the correctness of the reception using 
// Block Error Rate (BLER) curves.
// The PHY layer also measures the uplink and downlink channel quality by computing Channel Status 
// Information (CSI) of User Equipments (UEs) in both uplink and downlink direction. 
// Handover management is also responsibility of the PHY layer.    
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

