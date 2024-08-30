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

package simu5g.stack.mac;

//
// Interface for the Medium Access Control (MAC) layer of LTE Stack.
// 
// The LteMac moduleinterface provides the definition for the MAC layer in the LTE protocol stack.
// The MAC layer sends/receives MAC Service Data Units (SDUs) from the Radio Link Control (RLC) 
// layer and sends/receives MAC Protocol Data Units (PDUs) to the physical layer. The MAC layer
// is responsible to schedule MAC PDUs from different logical connections into the available
// bandwidth, in both uplink and downlink direction.
// Dynamic allocation of Resource Blocks (RBs) is carried out by the eNodeB on a Transmission Time
// Interval (TTI) basis, i.e., every 1 millisecond, and is assisted by User Equipments' Channel 
// Status Information (CSI) to perform link adaptation, and Buffer Status Reports (BSRs, for 
// uplink connections only). A Random Access procedure (RACH) is used by unscheduled UEs to 
// request uplink resources. Scheduling policies are defined separately by dedicated C++ classes.
// The MAC layer also provides Hybrid Automatic Repeat reQuest (H-ARQ) functionalities, enabling
// feedback reporting and retransmission of incorrectly-received MAC PDUs.
// Semi-persistent Scheduling (SPS), power control and Discountinuous Reception (DRX) are not
// implemented. 
//
moduleinterface ILteMac
{
    parameters:
        @display("i=block/mac");
        string interfaceTableModule;

    gates:
        input RLC_to_MAC;    // RLC to MAC
        output MAC_to_RLC;    // MAC to RLC
        input PHY_to_MAC;    // PHY to MAC
        output MAC_to_PHY;    // MAC to PHY
}

