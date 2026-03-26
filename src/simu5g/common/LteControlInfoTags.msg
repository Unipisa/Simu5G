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

import inet.common.INETDefs;
import inet.common.TagBase;
import inet.common.geometry.Geometry; // for Coord
import inet.networklayer.contract.ipv4.Ipv4Address;
import simu5g.common.LteCommon;
import simu5g.stack.mac.amc.UserTxParams;

namespace simu5g;

//
// Indication tag containing IP flow information
// Usually sent by Ip2Nic to PDCP.
//
class IpFlowInd extends inet::TagBase {
    inet::Ipv4Address srcAddr;          // Source IP address
    inet::Ipv4Address dstAddr;          // Destination IP address
    uint16 typeOfService = 0;           // IP Type of Service field
}

//
// Indication tag containing source and destination node identifiers
// used throughout the protocol stack.
// Normally exchanged by the PHY and MAC layers.
//
class NodeIdentificationInd extends inet::TagBase {
    MacNodeId sourceId = NODEID_NONE;     // Source MAC Node ID
    MacNodeId destId = NODEID_NONE;       // Destination MAC Node ID
}

//
// Indication tag containing traffic direction (UL/DL/D2D)  and D2D peer information
// Usually sent by PDCP to lower layers.
//
class TrafficDirectionInd extends inet::TagBase {
    unsigned short direction enum(Direction);  // Traffic Direction (UL, DL, D2D)
    MacNodeId d2dTxPeerId = NODEID_NONE;       // D2D transmitter peer ID
    MacNodeId d2dRxPeerId = NODEID_NONE;       // D2D receiver peer ID
}

//
// Indication tag containing traffic class for QoS handling.
// Usually sent by PDCP to MAC layer.
//
class TrafficClassInd extends inet::TagBase {
    unsigned short traffic enum(LteTrafficClass) = 0;      // LTE Traffic Class
}

//
// Identifies the logical connection and multicast group for the packet.
// Usually sent by PDCP to RLC and MAC layers.
//
class LogicalConnectionInd extends inet::TagBase {
    uint16 lcid = 0;                    // Logical Connection Identifier
    int32 multicastGroupId = -1;        // Multicast group ID (-1 for unicast)
}

//
// Common base for RLC mode request and indication tags.
// Usually exchanged between PDCP and RLC layers.
//
class RlcConfigurationTagBase extends inet::TagBase {
    unsigned short rlcType enum(LteRlcType) = 0;  // RLC mode (TM, UM, AM)
}

//
// Request tag that specifies the RLC mode (TM/UM/AM) for the connection.
// Usually sent by PDCP to RLC layer.
//
class RlcConfigurationReq extends RlcConfigurationTagBase {
}

//
// Indication tag containing the RLC mode (TM/UM/AM) for the connection.
// Usually sent by PDCP to RLC and MAC layers.
//
class RlcConfigurationInd extends RlcConfigurationTagBase {
}

//
// Request tag that specifies whether to use NR or LTE.
//
// Usually sent by Ip2Nic to PDCP.
//
class TechnologyReq extends inet::TagBase {
    bool useNR = false;                 // Use NR (5G) instead of LTE
}

//
// Indication tag containing carrier and technology configuration
// Normally exchanged by the PHY and MAC layers.
//
class CarrierConfigurationInd extends inet::TagBase {
    bool isNr = false;                  // Packet is for NR processing
    double carrierFrequency = NaN;      // Carrier frequency for transmission
}

//
// Indication tag containing HARQ process information
// Normally exchanged by the PHY and MAC layers.
//
class HarqInfoInd extends inet::TagBase {
    unsigned char acid = 0;             // HARQ process identifier
    unsigned char cw = 0;               // Codeword identifier
    unsigned char txNumber = 0;         // Number of (re)transmissions
    bool ndi = true;                    // New Data Indicator
}

//
// Indication tag containing PHY layer transmission parameters
// Normally exchanged by the PHY and MAC layers.
//
class PhyTransmissionInd extends inet::TagBase {
    unsigned short txMode = 0 enum(TxMode);             // Transmission mode
    unsigned int frameType = 0 enum(LtePhyFrameType);   // Frame type
    double txPower = NaN;                               // Transmission power
    double d2dTxPower = NaN;                            // D2D transmission power
    unsigned int totalGrantedBlocks = 0;                // Total granted resource blocks
    unsigned int grantId = 0;                           // Grant identifier
    inet::Coord coord;                                  // Playground position of the sending host
    RbMap grantedBlocks;                                // Resource blocks granted on all Remotes, all Bands
    FeedbackRequest feedbackReq;                        // Feedback request information
}

//
// Indication tag containing PDCP sequence number for packet ordering
// Usually sent by PDCP to RLC layer.
//
class SequenceNumberInd extends inet::TagBase {
    unsigned int sequenceNumber = 0;    // Packet PDCP sequence number
}

//
// Indication tag containing user transmission parameters
// Normally exchanged by the PHY and MAC layers.
//
class UserTransmissionParametersInd extends inet::TagBase {
    UserTxParams *userTxParams @owned;  // User transmission parameters
}

//
// Indication tag containing PHY layer reception and quality information
// Usually sent by PHY to MAC layer.
//
class PhyReceptionInd extends inet::TagBase {
    bool deciderResult = false;         // Decoding result
}

cplusplus (UserTransmissionParametersInd::setUserTxParams) {{
    // Code inserted at top of setUserTxParams(); allows silent overwrite of earlier object.
    delete this->userTxParams;
    this->userTxParams = nullptr;
}}
