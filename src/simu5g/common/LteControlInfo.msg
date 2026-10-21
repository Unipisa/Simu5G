//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
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
import inet.common.Units;
import simu5g.common.LteCommon;
import simu5g.common.LteCommonEnum;
import simu5g.stack.mac.amc.UserTxParams;

cplusplus {{
#include "simu5g/common/LteCommon.h"   // for FlowId, BearerRequest
}}

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
    MacNodeId d2dGroupId = NODEID_NONE;                  // D2D groupcast destination: the Binder's id for the multicast IP address; NODEID_NONE = not a groupcast flow
}

cplusplus (FlowControlInfo::setDrbId) {{
    ASSERT(num(drbId) <= MAX_DRB_ID);
}}

cplusplus(FlowControlInfo) {{
    // Build a tag from a FlowId. The tag carries identity only -- bearer configuration
    // (BearerRequest) is a separate, explicit argument at establishment time, never tag state.
    static FlowControlInfo fromFlowId(const FlowId& flow) {
        FlowControlInfo info;
        info.setSourceId(flow.sourceId);
        info.setDestId(flow.destId);
        info.setDirection(flow.direction);
        info.setD2dTxPeerId(flow.d2dTxPeerId);
        info.setD2dRxPeerId(flow.d2dRxPeerId);
        info.setDrbId(flow.drbId);
        info.setD2dGroupId(flow.d2dGroupId);
        return info;
    }
    // Extract the identity half of this tag as a FlowId.
    FlowId toFlowId() const {
        FlowId flow;
        flow.sourceId = getSourceId();
        flow.destId = getDestId();
        flow.direction = (Direction)getDirection();
        flow.d2dTxPeerId = getD2dTxPeerId();
        flow.d2dRxPeerId = getD2dRxPeerId();
        flow.drbId = getDrbId();
        flow.d2dGroupId = getD2dGroupId();
        return flow;
    }
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
// - PHY Control Information: transmission mode, frame type, transmit power,
//   resource blocks used (per remote antenna and band), sender position and
//   feedback request
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

    unsigned short txMode = 0 enum(TxMode);             // Transmission mode
    unsigned int frameType = 0 enum(LtePhyFrameType);   // Frame Type
    double txPower = NaN;                               //Transmission Power
    double d2dTxPower = NaN;                            // D2D Transmission Power (used for feedback reporting of D2D links
    // blocks granted on all Remotes, all Bands
    unsigned int totalGrantedBlocks = 0;                // note: currently unread
    unsigned int grantId = 0;                           // grantId related to the grand that allowed the sending of a MEC PDU (used only for MAC PDU sent by UEs)

    UserTxParams *userTxParams @owned;
    RbMap grantedBlocks;
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
