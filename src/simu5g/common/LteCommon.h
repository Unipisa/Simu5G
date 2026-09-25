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

//
//  Description:
//  This file contains LTE typedefs and constants.
//  At the end of the file there are some utility functions.
//

#ifndef _LTE_LTECOMMON_H_
#define _LTE_LTECOMMON_H_

#include <algorithm>
#include <bitset>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <inet/common/packet/Packet.h>
#include <inet/common/Protocol.h>
#include <inet/common/Units.h>
#include <inet/common/geometry/Geometry_m.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/contract/ipv4/Ipv4Address.h>

#include "simu5g/common/LteDefs.h"
#include "simu5g/common/LteTypes.h"
#include "simu5g/common/LteCommonEnum_m.h"

namespace inet {
class PacketFilter;
}

using namespace omnetpp;

using inet::Hz;
using inet::GHz;

namespace simu5g {

class Binder;
class CellInfo;
class LteMacEnb;
class LteMacBase;
class PhyBase;
class StochasticChannelModel;
class LteControlInfo;
class FlowControlInfo;
class ExtCell;
class IBackgroundTrafficManager;
class BackgroundScheduler;
class TrafficGeneratorBase;

/**
 * Lte specific protocols
 */
class LteProtocol
{
  public:
    static const inet::Protocol pdcp;    // Packet Data Convergence Protocol
    static const inet::Protocol rlc;     // Radio Link Control
    static const inet::Protocol ltemac;  // LTE Medium Access Control
    static const inet::Protocol gtp;     // GPRS Tunneling Protocol
    static const inet::Protocol x2ap;    // X2AP Protocol
    static const inet::Protocol sdap;    // Service Data Adaptation Protocol
};

// Identity of one flow/bearer instance between two ends, as used to request bearer
// establishment. This is the identity half of FlowControlInfo as a plain value: the
// packet tag is constructed FROM it, not the other way around.
struct FlowId {
    MacNodeId sourceId = NODEID_NONE;
    MacNodeId destId = NODEID_NONE;
    Direction direction = DL;
    DrbId drbId = DRBID_NONE;
    MacNodeId d2dGroupId = NODEID_NONE;
    MacNodeId d2dTxPeerId = NODEID_NONE;
    MacNodeId d2dRxPeerId = NODEID_NONE;

    DrbKey txDrbKey() const;    // same logic as ctrlInfoToTxDrbKey (D2D groupcast keys by group)
    DrbKey rxDrbKey() const;    // same logic as ctrlInfoToRxDrbKey
    FlowId reversed() const;    // same swaps as Binder's (former) makeReverseFlowControlInfo
};

// Identity of an IP flow as the packet classifier sees it: the fields the flow -> DRB
// bindings of ~Ip2Nic are keyed by. Not a packet filter (yet): the fields match exactly,
// except that the plain-LTE stack stores a wildcard direction to keep its key
// direction-agnostic.
struct FlowBindingKey {
    inet::L3Address srcAddr;
    inet::L3Address dstAddr;
    uint16_t typeOfService = 0;
    Direction direction = DL;

    bool operator==(const FlowBindingKey& other) const {
        return srcAddr == other.srcAddr && dstAddr == other.dstAddr &&
               typeOfService == other.typeOfService && direction == other.direction;
    }

    // The same flow as seen from the other end: addresses swapped and direction
    // reversed (D2D and the wildcard direction map to themselves).
    FlowBindingKey reversed() const {
        Direction revDir = (direction == UL) ? DL : (direction == DL) ? UL : direction;
        return FlowBindingKey{dstAddr, srcAddr, typeOfService, revDir};
    }
};

struct FlowBindingKeyHash {
    static std::size_t hashAddress(const inet::L3Address& addr) {
        if (addr.getType() == inet::L3Address::IPv4)
            return std::hash<uint32_t>{}(addr.toIpv4().getInt());
        const uint32_t *w = addr.toIpv6().words();
        return std::hash<uint32_t>{}(w[0] ^ w[1] ^ w[2] ^ w[3]);
    }
    std::size_t operator()(const FlowBindingKey& key) const {
        std::size_t h1 = hashAddress(key.srcAddr);
        std::size_t h2 = hashAddress(key.dstAddr);
        std::size_t h3 = std::hash<uint16_t>{}(key.typeOfService);
        std::size_t h4 = std::hash<uint16_t>{}(uint16_t(key.direction));
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

// The configuration half of a bearer-establishment request: what the requester asks RRC
// to set the bearer up as. rlcMode = UNKNOWN_RLC_MODE means the requester states nothing;
// the bearer configurator then resolves the request from the bearer's definition entry
// before it reaches RRC. The D2D mode-switch path passes explicit values because it
// transports already-decided configuration.
struct BearerRequest {
    RlcMode rlcMode = UM;
    Lcg lcg = Lcg(0);   // logicalChannelGroup (mac-LogicalChannelConfig)

    // The flow whose classification triggered this establishment, if any: RRC binds it
    // to the bearer at both endpoints, so the reverse flow resolves to the same DRB
    // instead of establishing a second, parallel bearer. Absent when the requester
    // classifies by other means (SDAP maps QFIs) or establishes bearers up front
    // (the bearer configurator's static definitions).
    std::optional<FlowBindingKey> flowBindingKey;
};

/**
 * TODO
 */
#define ELEM(x)            { x, #x }

/// Transmission time interval
#define TTI                0.001

/// Current simulation time
#define NOW                simTime()

/// Max Number of Codewords
#define MAX_CODEWORDS      2

// Number of QCI classes
#define LTE_QCI_CLASSES    9


// specifies a list of bands that can be used by a user
typedef std::vector<Band> UsableBands;

// maps a user with a set of usable bands. If a UE is not in the list, the set of usable bands comprises the whole spectrum
typedef std::map<MacNodeId, UsableBands> UsableBandsList;

/// Codeword
typedef unsigned short Codeword;

/// Numerology Index
typedef unsigned short NumerologyIndex;

// Attenuation vector for analogue models
typedef std::vector<double> AttenuationVector;


/*************************
*   Transmission Modes  *
*************************/

/// Number of transmission modes in DL direction.
const unsigned char DL_NUM_TXMODE = 6; //UNKNOWN_TX_MODE;

/// Number of transmission modes in UL direction.
const unsigned char UL_NUM_TXMODE = 6; //UNKNOWN_TX_MODE;


const unsigned int txModeToIndex[2] = { 0, 1 };

const TxMode indexToTxMode[2] = {
    SINGLE_ANTENNA_PORT0,
    TRANSMIT_DIVERSITY
};

typedef std::map<MacNodeId, TxMode> TxModeMap;

const double cqiToByteTms[16] = { 0, 2, 3, 5, 11, 15, 20, 25, 36, 38, 49, 63, 72, 79, 89, 92 };

double dBmToLinear(double dbm);
double dBToLinear(double db);
double linearToDBm(double lin);
double linearToDb(double lin);


/*****************
* X2 Support
*****************/

class X2InformationElement;

/**
 * The Information Elements List, a list
 * of IEs contained inside a X2Message
 */
typedef std::list<X2InformationElement *> X2InformationElementsList;

/**
 * The following structure specifies a band and a byte amount which limits the schedulable data
 * on it.
 * If this limit is -1, it is considered an unlimited capping.
 * If this limit is -2, the band cannot be used.
 * Among other modules, the rtxAcid and grant methods of LteSchedulerEnb use this structure.
 */
struct BandLimit
{
    /// Band which the element refers to
    Band band_;
    /// Limit of bytes (per codeword) which can be requested for the current band
    std::vector<int> limit_;

    BandLimit() : band_(0)
    {
        limit_.resize(MAX_CODEWORDS, -1);
    }

    // default "from Band" constructor
    BandLimit(Band b) : band_(b)
    {
        limit_.resize(MAX_CODEWORDS, -1);
    }

    bool operator<(const BandLimit rhs) const
    {
        return limit_[0] > rhs.limit_[0];
    }

};

typedef std::vector<BandLimit> BandLimitVector;

/*****************
* LTE Constants
*****************/

const unsigned char MAXCW = 2;
const Cqi MAXCQI = 15;
const Cqi NOSIGNALCQI = 0;
const Rank NORANK = 1;
const Tbs CQI2ITBSSIZE = 29;
const unsigned int PDCP_HEADER_UM = 1;
const unsigned int PDCP_HEADER_AM = 2;
// Flat RLC header sizes used by the LTE TS 36.322 UM/AM path (and by the LTE AM STATUS
// PDU, see LteRlcAmRxEntity::sendStatusReportLte). These are fixed approximations: the LTE
// header/STATUS size does not vary with the SN field length, the segmentation state, or
// the NACK count. The NR TS 38.322 SO path instead sizes each PDU exactly via
// nrUmHeaderBytes()/nrAmHeaderBytes() below; making LTE AM equally faithful is future work.
const unsigned int RLC_HEADER_UM = 2;
const unsigned int RLC_HEADER_AM = 2;

// NR-SO RLC header size (bytes) by segment state, so the TX and the scheduler agree on the
// exact per-PDU header. snBits is the SN field length carried by this flow (UM 6/12, AM
// 12/18 per TS 38.322); the byte size is computed by octet-aligning the fixed fields + SN
// and adding the 16-bit SO on a non-first (continuation) segment.
enum NrUmSegState { NRUM_COMPLETE = 0, NRUM_FIRST = 1, NRUM_CONTINUATION = 2 };

// UM (TS 38.322 6.2.1.3): complete SDU = SI+R = 1B (no SN/SO); first segment = SI(2)+SN,
// byte-aligned (6-bit SN -> 1B, 12-bit -> 2B); continuation = that + SO(16b) (3B / 4B).
inline unsigned int nrUmHeaderBytes(NrUmSegState s, unsigned int snBits)
{
    if (s == NRUM_COMPLETE)
        return 1u;                          // SI + R, no SN
    unsigned int base = (2u + snBits + 7u) / 8u;            // SI(2) + SN, octet-aligned
    return (s == NRUM_CONTINUATION) ? base + 2u : base;     // + SO(16b) on a continuation
}


// AM (TS 38.322 6.2.1.4): an AMD PDU always carries the SN. complete/first = D/C+P+SI(4)+SN,
// byte-aligned (12-bit SN -> 2B, 18-bit -> 3B); continuation adds the SO(16b) (4B / 5B).
inline unsigned int nrAmHeaderBytes(NrUmSegState s, unsigned int snBits)
{
    unsigned int base = (4u + snBits + 7u) / 8u;            // D/C+P+SI(4) + SN, octet-aligned
    return (s == NRUM_CONTINUATION) ? base + 2u : base;     // + SO(16b) on a continuation
}
const unsigned int MAC_HEADER = 2;
const unsigned int MAXGRANT = 4294967295U;

/*****************
* MAC Support
*****************/

class LteMacBuffer;
class LteMacQueue;
class MacControlElement;
class LteMacPdu;

/**
 * This is the Schedule list, a list of schedule elements.
 * For each CID on each codeword there is a number of SDUs
 */
typedef std::map<std::pair<MacCid, Codeword>, unsigned int> LteMacScheduleList;

/**
 * This is the Pdu list, a list of scheduled Pdus for
 * each user on each codeword.
 */
typedef std::map<std::pair<MacNodeId, Codeword>, inet::Packet *> MacPduList;

/*
 * Codeword list : for each node, it keeps track of allocated codewords (number)
 */
typedef std::map<MacNodeId, unsigned int> LteMacAllocatedCws;

/**
 * The Rlc Sdu List, a list of RLC SDUs
 * contained inside a RLC PDU
 */
typedef std::list<inet::Packet *> RlcSduList;
typedef std::list<size_t> RlcSduListSizes;

/**
 * The Mac Sdu List, a list of MAC SDUs
 * contained inside a MAC PDU
 */
typedef cPacketQueue MacSduList;

/**
 * The Mac Control Elements List, a list
 * of CEs contained inside a MAC PDU
 */
typedef std::list<MacControlElement *> MacControlElementsList;

/**
 * Set of MacNodeIds
 */
typedef std::set<MacNodeId> UeSet;

/*****************
* HARQ Support
*****************/

/// Unknown acid code
#define HARQ_NONE                255

/// time interval between two transmissions of the same pdu
#define HARQ_TX_INTERVAL         7 * TTI

// Codeword List - returned by Harq functions
typedef std::list<Codeword> CwList;

/// Pair of acid, list of unit ids
typedef std::pair<unsigned char, CwList> UnitList;

/*********************
* Incell Interference Support
*********************/
struct EnbInfo
{
    bool init;         // initialization flag
    bool isNr;        // eNodeB or gNodeB
    EnbType type;     // MICRO_ENB or MACRO_ENB
    double txPwr;
    TxDirectionType txDirection;
    double txAngle;
    MacNodeId id;
    PhyBase *phy = nullptr;
    LteMacEnb *mac = nullptr;
    StochasticChannelModel *realChan = nullptr;
    opp_component_ptr<cModule> eNodeB;
    int x2;
    std::string str() const;
};

struct UeInfo
{
    bool init;         // initialization flag
    double txPwr;
    MacNodeId id;
    MacNodeId cellId;
    StochasticChannelModel *realChan = nullptr;
    opp_component_ptr<cModule> ue;
    PhyBase *phy = nullptr;
    std::string str() const;
};

/**********************************
 * Background UEs avg CQI support *
 *********************************/
struct BgTrafficManagerInfo
{
    bool init;         // initialization flag
    IBackgroundTrafficManager *bgTrafficManager = nullptr;
    GHz carrierFrequency;
    double allocatedRbsDl;
    double allocatedRbsUl;
    std::vector<double> allocatedRbsUeUl;
    std::string str() const;
};

// uplink interference support
struct UeAllocationInfo {
    MacNodeId nodeId;
    MacCellId cellId;
    PhyBase *phy = nullptr;
    TrafficGeneratorBase *trafficGen = nullptr;
    Direction dir;
};

typedef std::vector<ExtCell *> ExtCellList;
typedef std::vector<BackgroundScheduler *> BackgroundSchedulerList;

/*****************
*  PHY Support  *
*****************/

typedef std::vector<std::vector<std::vector<double>>> BlerCurves;

struct SlotFormat {
    bool tdd;
    unsigned int numDlSymbols;
    unsigned int numUlSymbols;
    unsigned int numFlexSymbols;
};

struct CarrierInfo {
    GHz carrierFrequency;
    unsigned int numBands;
    unsigned int firstBand;
    unsigned int lastBand;
    BandLimitVector bandLimit;
    NumerologyIndex numerologyIndex;
    SlotFormat slotFormat;
};
typedef std::map<GHz, CarrierInfo> CarrierInfoMap;

/*************************************
* Shortcut for structures using STL
*************************************/

typedef std::vector<Cqi> CqiVector;
typedef std::set<Band> BandSet;
typedef std::set<Remote> RemoteSet;
typedef std::map<MacNodeId, bool> ConnectedUesMap;
typedef std::pair<int, simtime_t> PacketInfo;
typedef std::set<MacNodeId> ActiveUser;
typedef std::set<MacCid> ActiveSet;

/**
 * Used at initialization to pass the parameters
 * to the AnalogueModels and Decider.
 *
 * Parameters read from xml file are stored in this map.
 */
typedef std::map<std::string, cMsgPar> ParameterMap;

/*********************
* Utility functions
*********************/

const std::string dirToA(Direction dir);
/// Maps a BSR logical channel id to the traffic direction it announces
/// (reserved D2D BSR LCIDs promote to the D2D directions); returns 'fallback' otherwise.
Direction directionFromBsrLcid(LogicalCid lcid, Direction fallback);
const std::string d2dModeToA(LteD2DMode mode);
const std::string allocationTypeToA(RbAllocationType type);
const std::string modToA(LteMod mod);
const std::string periodicityToA(FbPeriodicity per);
const std::string txModeToA(TxMode tx);
TxMode aToTxMode(std::string s);
const std::string schedDisciplineToA(SchedDiscipline discipline);
SchedDiscipline aToSchedDiscipline(std::string s);
Remote aToDas(std::string s);
const std::string dasToA(const Remote r);
const char *nodeTypeToA(RanNodeType t);
RanNodeType aToNodeType(std::string name);
RanNodeType getNodeTypeById(MacNodeId id);
bool isBaseStation(CoreNodeType nodeType);
void verifyControlInfo(const FlowControlInfo *info);
bool isNrUe(MacNodeId id);
FeedbackType getFeedbackType(std::string s);
RbAllocationType getRbAllocationType(std::string s);
const std::string phyFrameTypeToA(const LtePhyFrameType r);
LtePhyFrameType aToPhyFrameType(std::string s);
const std::string rlcModeToA(RlcMode type);
char *cStringToLower(char *str);
RlcMode aToRlcMode(std::string s);
const std::string pduSessionTypeToA(PduSessionType type);
PduSessionType aToPduSessionType(std::string s);
const std::string coreNetworkToA(CoreNetwork type);
CoreNetwork aToCoreNetwork(std::string s);
const std::string cellGroupToA(CellGroup group);
CellGroup aToCellGroup(std::string s);

// Configure an inet::PacketFilter from its string form: an expression written as
// "expr(...)", or a message-name pattern. Parse errors throw here, at setup time.
void configurePacketFilter(inet::PacketFilter& filter, const char *spec);

// Take the expr() value of a policy parameter as an evaluatable expression, bound to
// the given variable resolver (whose ownership passes to the expression). The returned
// expression is the caller's to delete. Parameters holding anything but an expr()
// throw here, at setup time.
omnetpp::cDynamicExpression *getExpressionFromPar(omnetpp::cPar& par, omnetpp::cDynamicExpression::IResolver *resolver);
const std::string planeToA(Plane p);
MacNodeId ctrlInfoToUeId(const FlowControlInfo *info);
DrbKey ctrlInfoToTxDrbKey(const FlowControlInfo *info); // DrbKey for TX context: key by dest (remote receiver)
DrbKey ctrlInfoToRxDrbKey(const FlowControlInfo *info); // DrbKey for RX context: key by source (remote sender)
const std::string DeploymentScenarioToA(DeploymentScenario type);
DeploymentScenario aToDeploymentScenario(std::string s);

/**
 * Check if a MacNodeId represents a multicast destination ID
 *
 * @param nodeId the MacNodeId to check
 * @return true if the nodeId is in the multicast destination ID range
 */
inline bool isMulticastDestId(MacNodeId nodeId) {
    return num(nodeId) >= MULTICAST_DEST_MIN_ID;
}

/**
 * Utility function that reads the parameters of an XML element
 * and stores them in the passed ParameterMap reference.
 *
 * @param xmlData XML parameters config element related to a specific section
 * @param[output] outputMap map to store read parameters
 */
void getParametersFromXML(cXMLElement *xmlData, ParameterMap& outputMap);

/**
 * Parses a CSV string parameter into an int array.
 *
 * The string must be in the format "v1,v2,..,vi,...,vN;" and if dim > N,
 * the values[i] are filled with zeros from when i = N + 1.
 * Warning messages are issued if the string has less or more values than dim.
 *
 * @param str string to be parsed
 * @param values pointer to the int array
 * @param dim dimension of the values array
 * @param pad default value to be used when the string has less than dim values
 */
void parseStringToIntArray(std::string str, int *values, int dim, int pad);

/**
 * Initializes module's channels
 *
 * A dynamically created node needs its channels to be initialized, this method
 * runs through all a module's and its submodules' channels recursively and
 * initializes all channels.
 *
 * @param mod module whose channels needs initialization
 */
void initializeAllChannels(cModule *mod);

void removeAllSimu5GTags(inet::Packet *pkt);

template<typename T>
bool checkIfHeaderType(const inet::Packet *pkt, bool checkFirst = false) {

    auto pktAux = pkt->dup();

    int index = 0;
    while (pktAux->getBitLength() > 0 && pktAux->peekAtFront<inet::Chunk>()) {
        auto chunk = pktAux->popAtFront<inet::Chunk>();
        if (inet::dynamicPtrCast<const T>(chunk)) {
            delete pktAux;
            if (index != 0 && checkFirst)
                throw cRuntimeError("The header is not the top");
            return true;
        }
        index++;
    }
    delete pktAux;
    return false;
}

} //namespace

#endif
