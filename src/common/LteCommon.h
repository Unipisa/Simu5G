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

//
//  Description:
//  This file contains LTE typedefs and constants.
//  At the end of the file there are some utility functions.
//

#ifndef _LTE_LTECOMMON_H_
#define _LTE_LTECOMMON_H_

#define _NO_W32_PSEUDO_MODIFIERS

#include <algorithm>
#include <bitset>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include <omnetpp.h>
#include <inet/common/geometry/common/Coord.h>
#include <inet/common/packet/Packet.h>
#include <inet/common/Protocol.h>

#include "common/features.h"
#include "common/LteCommonEnum_m.h"

namespace simu5g {

using namespace omnetpp;

class Binder;
class CellInfo;
class LteCellInfo;
class LteNodeTable;
class LteMacEnb;
class LteMacBase;
class LtePhyBase;
class LteRealisticChannelModel;
class LteControlInfo;
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
    static const inet::Protocol ipv4uu;  // IP protocol on the uU interface
    static const inet::Protocol pdcp;    // Packet Data Convergence Protocol
    static const inet::Protocol rlc;     // Radio Link Control
    static const inet::Protocol ltemac;  // LTE Medium Access Control
    static const inet::Protocol gtp;     // GPRS Tunneling Protocol
    static const inet::Protocol x2ap;    // X2AP Protocol
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

/// MAC node ID
enum class MacNodeId : unsigned short {};  // emulate "strong typedef" with enum class

// Facilitates finding places where the numeric value of MacNodeId is used
inline unsigned short num(MacNodeId id) { return static_cast<unsigned short>(id); }

inline std::ostream& operator<<(std::ostream& os, MacNodeId id) { os << static_cast<unsigned short>(id); return os; }

// The following operators are mostly for simplifying comparisons and transformations involving UE_MIN_ID and similar constants
inline bool operator<(MacNodeId a, MacNodeId b) { return static_cast<unsigned short>(a) < static_cast<unsigned short>(b); }
inline bool operator>(MacNodeId a, MacNodeId b) { return static_cast<unsigned short>(a) > static_cast<unsigned short>(b); }
inline bool operator<=(MacNodeId a, MacNodeId b) { return static_cast<unsigned short>(a) <= static_cast<unsigned short>(b); }
inline bool operator>=(MacNodeId a, MacNodeId b) { return static_cast<unsigned short>(a) >= static_cast<unsigned short>(b); }
inline MacNodeId operator+(MacNodeId a, unsigned int b) { return MacNodeId(static_cast<unsigned short>(a) + b); }
inline MacNodeId operator-(MacNodeId a, unsigned int b) { return MacNodeId(static_cast<unsigned short>(a) - b); }
inline MacNodeId operator-(unsigned int a, MacNodeId b) { return MacNodeId(a - static_cast<unsigned short>(b)); }
inline unsigned short operator-(MacNodeId a, MacNodeId b) { return static_cast<unsigned short>(a) - static_cast<unsigned short>(b); }

/// Node Id bounds
constexpr MacNodeId NODEID_NONE  = MacNodeId(0);
constexpr MacNodeId ENB_MIN_ID   = MacNodeId(1);
constexpr MacNodeId ENB_MAX_ID   = MacNodeId(1023);
constexpr MacNodeId BGUE_ID      = MacNodeId(1024);
constexpr MacNodeId UE_MIN_ID    = MacNodeId(1025);
constexpr MacNodeId NR_UE_MIN_ID = MacNodeId(2049);
constexpr MacNodeId BGUE_MIN_ID  = MacNodeId(4097);
constexpr MacNodeId UE_MAX_ID    = MacNodeId(65535);


/// Cell node ID. It is numerically equal to eNodeB MAC node ID.
typedef MacNodeId MacCellId;

/// X2 node ID. It is equal to the eNodeB MAC Cell ID
typedef MacNodeId X2NodeId;

/// Omnet Node Id
typedef unsigned int OmnetId;

/// Logical Connection Identifier
typedef unsigned short LogicalCid;

/// Connection Identifier: <MacNodeId,LogicalCid>
typedef unsigned int MacCid;

/// Rank Indicator
typedef unsigned short Rank;

/// Channel Quality Indicator
typedef unsigned short Cqi;

/// Precoding Matrix Index
typedef unsigned short Pmi;

/// Transport Block Size
typedef unsigned short Tbs;

/// Logical band
typedef unsigned short Band;

// specifies a list of bands that can be used by a user
typedef std::vector<Band> UsableBands;

// maps a user with a set of usable bands. If a UE is not in the list, the set of usable bands comprises the whole spectrum
typedef std::map<MacNodeId, UsableBands> UsableBandsList;

/// Codeword
typedef unsigned short Codeword;

/// Numerology Index
typedef unsigned short NumerologyIndex;

/// identifies a traffic flow template
typedef int TrafficFlowTemplateId;

// QCI traffic descriptor
struct QCIParameters
{
    int priority;
    double packetDelayBudget;
    double packetErrorLossRate;
};

// Attenuation vector for analogue models
typedef std::vector<double> AttenuationVector;

struct ApplicationTable
{
    ApplicationType app;
    std::string appName;
};

const ApplicationTable applications[] = {
    ELEM(VOIP),
    ELEM(VOD),
    ELEM(WEB),
    ELEM(CBR),
    ELEM(FTP),
    ELEM(FULLBUFFER),
    ELEM(UNKNOWN_APP)
};

/**************************
* Scheduling discipline  *
**************************/

struct SchedDisciplineTable
{
    SchedDiscipline discipline;
    std::string disciplineName;
};

const SchedDisciplineTable disciplines[] = {
    ELEM(DRR),
    ELEM(PF),
    ELEM(MAXCI),
    ELEM(MAXCI_MB),
    ELEM(MAXCI_OPT_MB),
    ELEM(MAXCI_COMP),
    ELEM(ALLOCATOR_BESTFIT),
    ELEM(UNKNOWN_DISCIPLINE)
};

/*************************
*   Transmission Modes  *
*************************/
struct TxTable
{
    TxMode tx;
    std::string txName;
};

const TxTable txmodes[] = {
    ELEM(SINGLE_ANTENNA_PORT0),
    ELEM(SINGLE_ANTENNA_PORT5),
    ELEM(TRANSMIT_DIVERSITY),
    ELEM(OL_SPATIAL_MULTIPLEXING),
    ELEM(CL_SPATIAL_MULTIPLEXING),
    ELEM(MULTI_USER),
    ELEM(UNKNOWN_TX_MODE)
};

struct TxDirectionTable
{
    TxDirectionType txDirection;
    std::string txDirectionName;
};

const TxDirectionTable txDirections[] = {
    ELEM(ANISOTROPIC),
    ELEM(OMNI)
};

struct FeedbackRequest
{
    bool request;
    FeedbackGeneratorType genType;
    FeedbackType type;
    //used if genType==real
    TxMode txMode;
    bool dasAware;
    RbAllocationType rbAllocationType;
};

struct FeedbackGeneratorTypeTable
{
    FeedbackGeneratorType ty;
    std::string tyname;
};

const FeedbackGeneratorTypeTable feedbackGeneratorTypeTable[] = {
    ELEM(IDEAL),
    ELEM(REAL),
    ELEM(DAS_AWARE),
    ELEM(UNKNOW_FB_GEN_TYPE)
};

/// Number of transmission modes in DL direction.
const unsigned char DL_NUM_TXMODE = MULTI_USER + 1;

/// Number of transmission modes in UL direction.
const unsigned char UL_NUM_TXMODE = MULTI_USER + 1;

struct DeploymentScenarioMapping
{
    DeploymentScenario scenario;
    std::string scenarioName;
};

const DeploymentScenarioMapping DeploymentScenarioTable[] = {
    ELEM(INDOOR_HOTSPOT),
    ELEM(URBAN_MICROCELL),
    ELEM(URBAN_MACROCELL),
    ELEM(RURAL_MACROCELL),
    ELEM(SUBURBAN_MACROCELL),
    ELEM(UNKNOW_SCENARIO)
};

const unsigned int txModeToIndex[6] = { 0, 0, 1, 2, 2, 0 };

const TxMode indexToTxMode[3] = {
    SINGLE_ANTENNA_PORT0,
    TRANSMIT_DIVERSITY,
    OL_SPATIAL_MULTIPLEXING
};

typedef std::map<MacNodeId, TxMode> TxModeMap;

const double cqiToByteTms[16] = { 0, 2, 3, 5, 11, 15, 20, 25, 36, 38, 49, 63, 72, 79, 89, 92 };

struct Lambda
{
    unsigned int index;
    unsigned int lambdaStart;
    unsigned int channelIndex;
    double lambdaMin;
    double lambdaMax;
    double lambdaRatio;
};

double dBmToLinear(double dbm);
double dBToLinear(double db);
double linearToDBm(double lin);
double linearToDb(double lin);

/*************************
*      DAS Support      *
*************************/

struct RemoteTable
{
    Remote remote;
    std::string remoteName;
};

const RemoteTable remotes[] = {
    ELEM(MACRO),
    ELEM(RU1),
    ELEM(RU2),
    ELEM(RU3),
    ELEM(RU4),
    ELEM(RU5),
    ELEM(RU6),
    ELEM(UNKNOWN_RU)
};

/**
 * Maximum number of available DAS RU per cell.
 * To increase this number, change former enumerate accordingly.
 * MACRO antenna excluded.
 */
const unsigned char NUM_RUS = RU6;

/**
 * Maximum number of available ANTENNAS per cell.
 * To increase this number, change former enumerate accordingly.
 * MACRO antenna included.
 */
const unsigned char NUM_ANTENNAS = NUM_RUS + 1;

/**
 *  Block allocation Map: # of Rbs per Band, per Remote.
 */
typedef std::map<Remote, std::map<Band, unsigned int>> RbMap;

struct LtePhyFrameTable
{
    LtePhyFrameType phyType;
    std::string phyName;
};

const LtePhyFrameTable phytypes[] = {
    ELEM(DATAPKT),
    ELEM(BROADCASTPKT),
    ELEM(FEEDBACKPKT),
    ELEM(HANDOVERPKT),
    ELEM(GRANTPKT),
    ELEM(D2DMODESWITCHPKT),
    ELEM(UNKNOWN_TYPE)
};

struct RanNodeTable
{
    RanNodeType node;
    std::string nodeName;
};

const RanNodeTable nodetypes[] = {
    ELEM(INTERNET),
    ELEM(ENODEB),
    ELEM(GNODEB),
    ELEM(UE),
    ELEM(UNKNOWN_NODE_TYPE)
};

//|--------------------------------------------------|

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
const Pmi NOPMI = 0;
const Rank NORANK = 1;
const Tbs CQI2ITBSSIZE = 29;
const unsigned int PDCP_HEADER_UM = 1;
const unsigned int PDCP_HEADER_AM = 2;
const unsigned int RLC_HEADER_UM = 2; // TODO
const unsigned int RLC_HEADER_AM = 2; // TODO
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
 * This is a map that associates each Connection Id with
 * a Mac Queue, storing  MAC SDUs (or RLC PDUs)
 */
typedef std::map<MacCid, LteMacQueue *> LteMacBuffers;

/**
 * This is a map that associates each Connection Id with
 *  a buffer storing the  MAC SDUs info (or RLC PDUs).
 */
typedef std::map<MacCid, LteMacBuffer *> LteMacBufferMap;

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

/// Number of harq tx processes
#define ENB_TX_HARQ_PROCESSES    8
#define UE_TX_HARQ_PROCESSES     8
#define ENB_RX_HARQ_PROCESSES    8
#define UE_RX_HARQ_PROCESSES     8

/// time interval between two transmissions of the same pdu
#define HARQ_TX_INTERVAL         7 * TTI

struct RemoteUnitPhyData
{
    int txPower;
    inet::Coord m;
};

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
    RanNodeType nodeType; // ENODEB or GNODEB
    EnbType type;     // MICRO_ENB or MACRO_ENB
    double txPwr;
    TxDirectionType txDirection;
    double txAngle;
    MacNodeId id;
    LtePhyBase *phy = nullptr;
    LteMacEnb *mac = nullptr;
    LteRealisticChannelModel *realChan = nullptr;
    opp_component_ptr<cModule> eNodeB;
    int x2;
};

struct UeInfo
{
    bool init;         // initialization flag
    double txPwr;
    MacNodeId id;
    MacNodeId cellId;
    LteRealisticChannelModel *realChan = nullptr;
    opp_component_ptr<cModule> ue;
    LtePhyBase *phy = nullptr;
};

/**********************************
 * Background UEs avg CQI support *
 *********************************/
struct BgTrafficManagerInfo
{
    bool init;         // initialization flag
    IBackgroundTrafficManager *bgTrafficManager = nullptr;
    double carrierFrequency;
    double allocatedRbsDl;
    double allocatedRbsUl;
    std::vector<double> allocatedRbsUeUl;
};

// uplink interference support
struct UeAllocationInfo {
    MacNodeId nodeId;
    MacCellId cellId;
    LtePhyBase *phy = nullptr;
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
    double carrierFrequency;
    unsigned int numBands;
    unsigned int firstBand;
    unsigned int lastBand;
    BandLimitVector bandLimit;
    NumerologyIndex numerologyIndex;
    SlotFormat slotFormat;
};
typedef std::map<double, CarrierInfo> CarrierInfoMap;

/*************************************
* Shortcut for structures using STL
*************************************/

typedef std::vector<Cqi> CqiVector;
typedef std::vector<Pmi> PmiVector;
typedef std::set<Band> BandSet;
typedef std::set<Remote> RemoteSet;
typedef std::map<MacNodeId, bool> ConnectedUesMap;
typedef std::pair<int, simtime_t> PacketInfo;
typedef std::vector<RemoteUnitPhyData> RemoteUnitPhyDataVector;
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
const std::string nodeTypeToA(const RanNodeType t);
RanNodeType aToNodeType(std::string name);
RanNodeType getNodeTypeById(MacNodeId id);
bool isBaseStation(CoreNodeType nodeType);
bool isNrUe(MacNodeId id);
FeedbackType getFeedbackType(std::string s);
RbAllocationType getRbAllocationType(std::string s);
ApplicationType aToApplicationType(std::string s);
const std::string applicationTypeToA(std::string s);
const std::string lteTrafficClassToA(LteTrafficClass type);
LteTrafficClass aToLteTrafficClass(std::string s);
const std::string phyFrameTypeToA(const LtePhyFrameType r);
LtePhyFrameType aToPhyFrameType(std::string s);
const std::string rlcTypeToA(LteRlcType type);
char *cStringToLower(char *str);
LteRlcType aToRlcType(std::string s);
const std::string planeToA(Plane p);
MacNodeId ctrlInfoToUeId(inet::Ptr<LteControlInfo> info);
MacCid idToMacCid(MacNodeId nodeId, LogicalCid lcid);
MacCid ctrlInfoToMacCid(inet::Ptr<LteControlInfo> info);        // get the CID from the packet control info
MacNodeId MacCidToNodeId(MacCid cid);
LogicalCid MacCidToLcid(MacCid cid);
CellInfo *getCellInfo(Binder *binder, MacNodeId nodeId);
cModule *getPhyByMacNodeId(Binder *binder, MacNodeId nodeId);
cModule *getMacByMacNodeId(Binder *binder, MacNodeId nodeId);
cModule *getRlcByMacNodeId(Binder *binder, MacNodeId nodeId, LteRlcType rlcType);
cModule *getPdcpByMacNodeId(Binder *binder, MacNodeId nodeId);
LteMacBase *getMacUe(Binder *binder, MacNodeId nodeId);
FeedbackGeneratorType getFeedbackGeneratorType(std::string s);
const std::string fbGeneratorTypeToA(FeedbackGeneratorType type);
const std::string DeploymentScenarioToA(DeploymentScenario type);
DeploymentScenario aToDeploymentScenario(std::string s);
bool isMulticastConnection(LteControlInfo *lteInfo);

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

template<typename T>
std::vector<T> getTagsWithInherit(inet::Packet *pkt)
{
    std::vector<T> t;
    auto tags = pkt->getTags();
    if (tags.getNumTags() == 0)
        return t;

    // check if a tag that is derived from this exists.
    //
    for (int i = 0; i < tags.getNumTags(); i++) {
        auto tag = tags.getTagForUpdate(i);
        auto temp = inet::dynamicPtrCast<T>(tag);
        if (temp != nullptr) {
            t.push_back(*temp.get());
        }
    }
    return t;
}

} //namespace

#endif

