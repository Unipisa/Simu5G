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

import inet.common.INETDefs;
import inet.common.TagBase;
import common.LteCommon;

namespace simu5g;

enum LtePhyFrameType;
enum LteRlcType;
enum TxMode;
enum Direction;
enum ApplicationType;
enum LteTrafficClass;

//
// Control info usage in Simu5G simulator
//
// - FlowControlInfo contains information with flow granularity.
//   It is used from Ip2Nic to Mac.
//
// - UserControlInfo contains information with user granularity.
//   It is used from Mac to Phy.
//
// Both FlowControlInfo and UserControlInfo inherit from LteControlInfo
// to avoid replication of common parts.
//
// The Mac layer receives packets (MAC sdu) with FlowControlInfo attached
// and creates a MAC pdu with UserControlInfo attached.
// The MacPduMaker does the job.
//
// NOTE: a MAC pdu contains different MAC sdus that may belong to different flows.
//

//
// @class LteControlInfo
// @brief Base class for FlowControlInfo and UserControlInfo
//
// This class contains information present in both Flow and User ControlInfo:
// - Source MAC Node ID
// - Destination MAC Node ID
//
class LteControlInfo extends inet::TagBase {
    //# Information shared between FlowControlInfo and UserControlInfo

    MacNodeId sourceId = NODEID_NONE;                    // Source MacNodeId
    MacNodeId destId = NODEID_NONE;                      // Destination MacNodeId
    unsigned short direction enum(Direction);            // Traffic Direction (UL, DL, D2D)
    MacNodeId d2dTxPeerId = NODEID_NONE;
    MacNodeId d2dRxPeerId = NODEID_NONE;

    //# Traffic Control Information

    unsigned short application enum(ApplicationType) = 0;    // Application Type
    unsigned short traffic enum(LteTrafficClass) = 0;        // Lte Traffic Type

    //# RLC Control Information

    unsigned short rlcType enum(LteRlcType) = 0;            // RLC Traffic Type

    //# Connection Information

    uint16 lcid = 0;                                        // Logical Connection Identifier
    int32 multicastGroupId = -1;                        // Identifier for a multicast group (it is in range [0:[2^28)-1] )
                                                        // For unicast communications, this field is -1
}

//
// @class FlowControlInfo
// @brief contains information flowing from LteIp to Mac
//
// It contains the following information:
// - IP Control Informations: the four tuple, a sequence number and
//   the header size (IP + Transport) to be used by pdcp ROHC.
// - Traffic Control Informations: Application, Direction, Traffic
// - RLC Control Informations: the traffic type (TM, UM, AM)
// - Connection information: Logical CID
//
class FlowControlInfo extends LteControlInfo {
    //# IP Control Information

    uint32 srcAddr = 0;                     // source IP
    uint32 dstAddr = 0;                     // destination IP
    uint16 typeOfService = 0;               // IP Type Of Service
    unsigned int sequenceNumber = 0;        // packet sequence number
    int headerSize = 0;                     // IP + Transport header size (used by pdcp ROHC)
    bool useNR = false;                     // flag for sending this packet using NR
}

//
// @class UserControlInfo
// @brief contains information flowing from Mac to Phy
//
// It contains the following information:
// - H-ARQ Control Information
// - PHY Control Informations: Broadcast, Corruption, Type,
//   txMode, Resource blocks used, RemoteSet
//
class UserControlInfo extends LteControlInfo {
    @customize(true);

    bool isNr = false;
    double carrierFrequency = NaN;       // carrier frequency

    //# H-ARQ Control Information

    unsigned char acid = 0;              // H-ARQ process identifier
    unsigned char cw = 0;                // H-ARQ codeword identifier
    unsigned char txNumber = 0;          // number of (re)transmissions for the same pdu (1, 2, 3, 4)
    bool ndi = true;                     // new data indicator (new data overwrites a process content if present)

    //# PHY Control Informations

    bool isCorruptible = false;                         // Frame is corruptible
    bool isBroadcast = false;                           // Broadcast packet
    bool deciderResult = false;                         // Decider result
    double blerTh = NaN;                                // Calculated BLER threshold for this transmission
    double blerValue = NaN;                             // Extracted BLER for this transmission (blerValue > blerTh ==> corrupted)
    unsigned short txMode = 0 enum(TxMode);             // Traffic Type
    unsigned int frameType = 0 enum(LtePhyFrameType);   // Frame Type
    double txPower = NaN;                               //Transmission Power
    double d2dTxPower = NaN;                            // D2D Transmission Power (used for feedback reporting of D2D links
    // blocks granted on all Remotes, all Bands
    unsigned int totalGrantedBlocks = 0;

    unsigned int grantId = 0;                           // grantId related to the grand that allowed the sending of a MEC PDU (used only for MAC PDU sent by UEs)

    //#
    //# Follows a list of elements only present in
    //# the customized class (see LteControlInfo.h):
    //#
    //# RemoteSet ru;                                // Remote Antenna Set
    //# RbMap grantedBlocks;                        // Blocks allocated per Remote, per Band.
}
