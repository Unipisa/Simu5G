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

namespace simu5g;

/// Link Directions
enum Direction
{
    DL = 0; UL = 1; D2D = 2; D2D_MULTI = 3; UNKNOWN_DIRECTION = 4;
};

/// Modulations
enum LteMod
{
    _QPSK = 0; _16QAM = 1; _64QAM = 2; _256QAM = 3;
};

/// Feedback reporting type
enum FeedbackType
{
    ALLBANDS = 0; PREFERRED = 1; WIDEBAND = 2;
};

/// Feedback periodicity
enum FbPeriodicity
{
    PERIODIC = 0; APERIODIC = 1;
};

/// Resource allocation type
enum RbAllocationType
{
    TYPE2_DISTRIBUTED = 0;
    TYPE2_LOCALIZED = 1;
};

/// Lte Traffic Classes. They are used as LCG IDs as sent in BSRs. LTE has 4 LCGs,
/// NR supports up to 8. This enum only defines 4, but this can be extended.
enum LteTrafficClass
{
    CONVERSATIONAL = 0; STREAMING =1;  INTERACTIVE = 2; BACKGROUND = 3; UNKNOWN_TRAFFIC_TYPE = 4;
};

/// Scheduler grant type
enum GrantType
{
    FITALL = 0; FIXED_ = 1; URGENT = 2; UNKNOWN_GRANT_TYPE = 3; // Note: FIXED would clash with <windows.h>
};

/// Lte RLC Types
enum LteRlcType
{
    TM = 0; UM = 1; AM = 2; UNKNOWN_RLC_TYPE = 3;
};

/// PDU Session Types (3GPP TS 23.501)
enum PduSessionType
{
    IP_V4 = 0; IP_V6 = 1; IP_V4V6 = 2; ETHERNET = 3; UNSTRUCTURED = 4;
};

/// Index for UL transmission map
enum UlTransmissionMapTTI
{
    PREV_TTI = 0; CURR_TTI = 1;
};

//
// D2D Modes:
// IM: Infastructure Mode
// DM: Direct (D2D) Mode
// NONE: Non D2D-capable
//
enum LteD2DMode
{
    NONE = 0;
    IM = 1;
    DM = 2;
};

//
//  Scheduling discipline
//
enum SchedDiscipline
{
    DRR = 0;
    PF = 1;
    MAXCI = 2;
    MAXCI_MB = 3;
    MAXCI_OPT_MB = 4;
    MAXCI_COMP = 5;
    ALLOCATOR_BESTFIT = 6;
    QOS_PF = 7;
    UNKNOWN_DISCIPLINE = 8;

};

// specifies how the final CQI will be computed from the multiband ones
enum PilotComputationModes
{
    MIN_CQI = 0;
    MAX_CQI = 1;
    AVG_CQI = 2;
    MEDIAN_CQI = 3;
    ROBUST_CQI = 4;
};


///   Transmission Modes
enum TxMode
{
    // Note: If you add more tx modes, update DL_NUM_TXMODE and UL_NUM_TXMODE
    SINGLE_ANTENNA_PORT0 = 0;
    TRANSMIT_DIVERSITY = 1;
    UNKNOWN_TX_MODE = 2;
};

enum TxDirectionType
{
    ANISOTROPIC = 0;
    OMNI = 1;
    UNKNOWN_TX_DIRECTION = -1;
};

/// OFDMA layers (see FIXME lteAllocationModuble.h for "layers" meaning)
enum Plane
{
    MAIN_PLANE = 0;
    MU_MIMO_PLANE = 1; // currently unused
};

enum DeploymentScenario
{
    INDOOR_HOTSPOT = 0;
    URBAN_MICROCELL = 1;
    URBAN_MACROCELL = 2;
    RURAL_MACROCELL = 3;
    SUBURBAN_MACROCELL = 4;
    UNKNOWN_SCENARIO = 5;
};


/// OFDMA Remotes Antennas (see LteAllocationModule) -- only MACRO is currently supported
enum Remote
{
    MACRO = 0;
    RU1 = 1;
    RU2 = 2;
    RU3 = 3;
    RU4 = 4;
    RU5 = 5;
    RU6 = 6;
    UNKNOWN_RU = 7;
};

// Lte PHY Frame Types
enum LtePhyFrameType
{
    DATAPKT = 0;
    BROADCASTPKT  = 1;
    FEEDBACKPKT = 2;
    BEACONPKT = 3;
    HARQPKT = 4;
    GRANTPKT = 5;
    RACPKT = 6;
    D2DMODESWITCHPKT = 7;
    UNKNOWN_TYPE = 8;
};

// Node types on the Radio Access Network. Note that the LTE vs 5G (NR)
// distinction is usually represented with isNr booleans and/or nodeId/nrNodeId
// variables being empty or not.
enum RanNodeType
{
    NODEB  = 0; // eNodeB / gNodeB
    UE = 1; // UE
    UNKNOWN_NODE_TYPE  = 2;
};


//  Core Network Types
enum CoreNodeType
{
    ENB = 0;          // LTE
    PGW = 1;
    SGW = 2;
    GNB = 3;          // 5G
    UPF = 4;
    UPF_MEC = 5;      // MEC
};


/// H-ARQ feedback (ACK, NACK)
enum HarqAcknowledgment
{
    HARQNACK = 0; HARQACK = 1;
};

/// TX H-ARQ pdu status
enum TxHarqPduStatus
{
    /// pdu is ready for retransmission (nack received)
    TXHARQ_PDU_BUFFERED = 0;
    /// pdu is waiting for feedback
    TXHARQ_PDU_WAITING = 1;
    /// no pdu inside this process (empty process)
    TXHARQ_PDU_EMPTY = 2;
    /// pdu selected for transmission
    TXHARQ_PDU_SELECTED = 3;
};

/// RX H-ARQ pdu status
enum RxHarqPduStatus
{
    /// no pdu, process is empty
    RXHARQ_PDU_EMPTY = 0;
    /// pdu is in evaluating state
    RXHARQ_PDU_EVALUATING = 1;
    /// pdu has been evaluated and it is correct
    RXHARQ_PDU_CORRECT = 2;
    /// pdu has been evaluated and it is not correct
    RXHARQ_PDU_CORRUPTED = 3;
};


//  ********************* * Incell Interference Support *********************
enum EnbType
{
    // macro eNb
    MACRO_ENB = 0;
    // micro eNb
    MICRO_ENB = 1;
};
