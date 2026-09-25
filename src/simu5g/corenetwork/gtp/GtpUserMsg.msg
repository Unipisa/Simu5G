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
import inet.common.packet.chunk.Chunk;
import simu5g.common.LteCommon;

namespace simu5g;

//
// The PDU Session Container extension header of a GTP-U header (TS 38.415 5.5.2),
// which carries the QFI of a G-PDU on N3: which PDU SESSION INFORMATION it holds, if any
//
enum PduSessionContainerType
{
    PDU_SESSION_CONTAINER_NONE = -1;   // No PDU Session Container
    DL_PDU_SESSION_INFORMATION = 0;    // PDU Type 0, sent by the core network
    UL_PDU_SESSION_INFORMATION = 1;    // PDU Type 1, sent by the RAN
}

//
// A GTP-U header (TS 29.281 5.1): the 8 mandatory octets, and when a PDU Session
// Container is present, the 4 optional octets that announce the extension header and
// the 4-octet PDU Session Container itself (TS 38.415): 16 octets then.
//
class GtpUserMsg extends inet::FieldsChunk {
    uint8_t messageType = 255;   // GTPU_G_PDU or GTPU_END_MARKER (TS 29.281 6.1)
    uint16_t lengthField;        // The octets after the mandatory header: optional fields, extension headers and the T-PDU
    Teid teid;                   // Allocated by the receiving end of the tunnel; TEID_NONE on relays between core network nodes, which carry no PDU session's traffic
    PduSessionContainerType pduSessionContainer = PDU_SESSION_CONTAINER_NONE;
    Qfi qfi;                     // The QoS flow; on the wire only inside the PDU Session Container
    chunkLength = inet::B(8);
}

cplusplus {{
// GTP-U message types (TS 29.281 6.1)
constexpr uint8_t GTPU_END_MARKER = 254;   // The last packet on a tunnel a PDU session's downlink leaves (TS 29.281 7.3.2)
constexpr uint8_t GTPU_G_PDU = 255;        // User data (a T-PDU)

// The length of a GTP-U header with or without a PDU Session Container
inline inet::B gtpUserHeaderLength(PduSessionContainerType container) { return inet::B(container == PDU_SESSION_CONTAINER_NONE ? 8 : 16); }
}}
