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
import inet.common.geometry.Geometry; // for Coord
import inet.common.Units;
import simu5g.common.LteCommon;
import simu5g.common.LteCommonEnum;
import simu5g.stack.mac.amc.UserTxParams;

namespace simu5g;

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
// - D2D MAC Node Ids
// - Traffic Direction
// - Connection information: LCID
//
class LteControlInfo extends inet::TagBase {
    MacNodeId sourceId = NODEID_NONE;                    // Source MacNodeId
    MacNodeId destId = NODEID_NONE;                      // Destination MacNodeId
    Direction direction = DL;                            // Traffic Direction (UL, DL, D2D)
    MacNodeId d2dTxPeerId = NODEID_NONE;
    MacNodeId d2dRxPeerId = NODEID_NONE;
}

//
// @class FlowControlInfo
// @brief Contains flow information in layers above the MAC
//
class FlowControlInfo extends LteControlInfo {
    DrbId drbId;                                         // DRB (Data Radio Bearer) identifier
    MacNodeId multicastGroupId = NODEID_NONE;            // Identifier for a multicast group (it is in range [0:[2^28)-1] ) -- TODO probably should not be needed
    int typeOfService; //TODO temporarily added field -- remove!!!

    unsigned short traffic enum(LteTrafficClass) = 0;       // Lte Traffic Type; analogous to LCG IDs (Logical Channel Group Identifiers) in LTE/NR
    unsigned short rlcType enum(LteRlcType) = 0;            // RLC Traffic Type
}

cplusplus (FlowControlInfo::setDrbId) {{
    ASSERT(num(drbId) <= 63);
}}

// WIP: Describes logical channels (identified by remote node + LCID) in MAC
class FlowDescriptor extends FlowControlInfo {
}

cplusplus(FlowDescriptor) {{
    static FlowDescriptor fromFlowControlInfo(const FlowControlInfo& ci) {
       return FlowDescriptor(*(FlowDescriptor*)&ci);
    }
    FlowControlInfo toFlowControlInfo() const {
       return FlowControlInfo(*this);
    }
}}

struct FeedbackRequest
{
    bool request;
    FeedbackType type;
    TxMode txMode;
    bool dasAware;
    RbAllocationType rbAllocationType;
};

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
    //TODO packetLcid and packetMulticastGroupId make no sense in UserControlInfo, eliminate
    LogicalCid packetLcid;                 //  only used to indicate BSR type in MAC
    MacNodeId packetMulticastGroupId = NODEID_NONE;     // Identifier for a multicast group (it is in range [0:[2^28)-1] ) -- TODO seems to be used instead of proper MBMS RNTIs stored in destId

    bool isNr = false;
    inet::GHz carrierFrequency;       // carrier frequency

    //# H-ARQ Control Information

    unsigned char acid = 0;              // H-ARQ process identifier
    unsigned char cw = 0;                // H-ARQ codeword identifier
    unsigned char txNumber = 0;          // number of (re)transmissions for the same pdu (1, 2, 3, 4)
    bool ndi = true;                     // new data indicator (new data overwrites a process content if present)

    //# PHY Control Informations

    unsigned short txMode = 0 enum(TxMode);             // Traffic Type
    unsigned int frameType = 0 enum(LtePhyFrameType);   // Frame Type
    double txPower = NaN;                               //Transmission Power
    double d2dTxPower = NaN;                            // D2D Transmission Power (used for feedback reporting of D2D links
    // blocks granted on all Remotes, all Bands
    unsigned int totalGrantedBlocks = 0;                // note: currently unread
    unsigned int grantId = 0;                           // grantId related to the grand that allowed the sending of a MEC PDU (used only for MAC PDU sent by UEs)

    UserTxParams *userTxParams @owned;
    RbMap grantedBlocks;
    inet::Coord coord;                                  // The playground position of the sending host
    FeedbackRequest feedbackReq;
}

cplusplus (UserControlInfo) {{
    virtual const unsigned int getBlocks(Remote antenna, Band b) const { return grantedBlocks.at(antenna).at(b); }
    virtual void setBlocks(Remote antenna, Band b, const unsigned int blocks) { grantedBlocks[antenna][b] = blocks; }
}}

cplusplus (UserControlInfo::setUserTxParams) {{
    // Code inserted at top of setUserTxParams(); allows silent overwrite of earlier object.
    delete this->userTxParams;
    this->userTxParams = nullptr;
}}
