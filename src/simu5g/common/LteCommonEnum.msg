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

/// Lte Traffic Classes
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

/// Index for UL transmission map
enum UlTransmissionMapTTI
{
    PREV_TTI = 0; CURR_TTI = 1;
};

/// D2D Modes
// IM = Infastructure Mode
// DM = Direct (D2D) Mode
enum LteD2DMode
{
    IM = 0; DM = 1;
};

///
//      Applications  
// 

enum ApplicationType
{
    VOIP = 0;
    VOD = 1;
    WEB = 2;
    CBR = 3;
    FTP = 4;
    GAMING = 5;
    FULLBUFFER = 6;
    UNKNOWN_APP = 7;
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
    UNKNOWN_DISCIPLINE = 7;
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
    SINGLE_ANTENNA_PORT5 =1;
    TRANSMIT_DIVERSITY = 2;
    OL_SPATIAL_MULTIPLEXING = 3;
    CL_SPATIAL_MULTIPLEXING = 4;
    MULTI_USER = 5;
    UNKNOWN_TX_MODE = 6;
};

enum TxDirectionType
{
    ANISOTROPIC = 0;
    OMNI = 1;
    UNKNOWN_TX_DIRECTION = -1;
};

// Lte feedback type
enum FeedbackGeneratorType
{
    IDEAL = 0;
    REAL = 1;
    DAS_AWARE = 2;
    UNKNOW_FB_GEN_TYPE = 3;
};

/// OFDMA layers (see FIXME lteAllocationModuble.h for "layers" meaning)
enum Plane
{
    MAIN_PLANE = 0;
    MU_MIMO_PLANE = 1;
};

enum DeploymentScenario
{
    INDOOR_HOTSPOT = 0;
    URBAN_MICROCELL = 1;
    URBAN_MACROCELL = 2;
    RURAL_MACROCELL = 3;
    SUBURBAN_MACROCELL = 4;
    UNKNOW_SCENARIO = 5;
};


//        DAS Support      
/// OFDMA Remotes (see FIXME LteAllocationModule.h for "antenna" meaning)
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
    HANDOVERPKT = 3;
    HARQPKT = 4;
    GRANTPKT = 5;
    RACPKT = 6;
    D2DMODESWITCHPKT = 7;
    UNKNOWN_TYPE = 8;
};


//  RAN Node Types  
enum RanNodeType
{
    INTERNET = 0; /// Internet side of the Lte network
    ENODEB  = 1; /// eNodeB
    GNODEB  = 2; /// gNodeB
    UE = 3; /// UE
    UNKNOWN_NODE_TYPE  = 4;/// unknown
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


// Types of BSR  * TODO add LONG/TRUNCATED BSR  
enum BsrType
{
    SHORT_BSR  = 0;
    D2D_SHORT_BSR = 1;
    D2D_MULTI_SHORT_BSR  = 2;
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
