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
import common.LteCommon;
import inet.common.TagBase;


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

    uint16 sourceId;                                     // Source MacNodeId
    uint16 destId;                                       // Destination MacNodeId
    unsigned short direction enum(Direction);            // Traffic Direction (UL, DL, D2D)
    uint16 d2dTxPeerId;
    uint16 d2dRxPeerId;

    //# Traffic Control Information

    unsigned short application enum(ApplicationType);    // Application Type
    unsigned short traffic enum(LteTrafficClass);        // Lte Traffic Type

    //# RLC Control Information

    unsigned short rlcType enum(LteRlcType);            // RLC Traffic Type

    //# Connection Information

    uint16 lcid;                                        // Logical Connection Identifier
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

    uint32 srcAddr;                                       // source IP
    uint32 dstAddr;                                       // destination IP
    uint16 typeOfService;								  // IP Type Of Service
    unsigned int sequenceNumber;                          // packet sequence number
    int headerSize;                                       // IP + Transport header size (used by pdcp ROHC)
    bool useNR = false;									  // flag for sending this packet using NR
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
    double carrierFrequency;         // carrier frequency

    //# H-ARQ Control Information

    unsigned char acid;                // H-ARQ process identifier
    unsigned char cw;                // H-ARQ codeword identifier
    unsigned char txNumber = 0;            // number of (re)transmissions for the same pdu (1, 2, 3, 4)
    bool ndi = true;                    // new data indicator (new data overwrites a process content if present)

    //# PHY Control Informations

    bool isCorruptible;                                // Frame is corruptible
    bool isBroadcast = false;                        // Broadcast packet
    bool deciderResult;                                // Decider result
    double blerTh;                                    // Calculated BLER threshold for this transmission
    double blerValue;                                // Extracted BLER for this transmission (blerValue > blerTh ==> corrupted)
    unsigned short txMode enum(TxMode);                // Traffic Type
    unsigned int frameType enum(LtePhyFrameType);    // Frame Type
    double txPower;                                    //Transmission Power
    double d2dTxPower;                                 //D2D Transmission Power (used for feedback reporting of D2D links
    // blocks granted on all Remotes, all Bands
    unsigned int totalGrantedBlocks;

    //#
    //# Follows a list of elements only present in
    //# the customized class (see LteControlInfo.h):
    //#
    //# RemoteSet ru;                                // Remote Antenna Set
    //# RbMap grantedBlocks;                        // Blocks allocated per Remote, per Band.
}


