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

#include <algorithm>
#include <bitset>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <inet/common/packet/Packet.h>
#include <inet/common/Protocol.h>
#include <inet/common/Units.h>

#include "simu5g/common/LteDefs.h"
#include "simu5g/common/LteCommonEnum_m.h"
#include "simu5g/common/LteCommon_m.h"

using namespace omnetpp;

using inet::Hz;
using inet::GHz;

namespace simu5g {

class Binder;
class CellInfo;
class LteMacEnb;
class LteMacBase;
class LtePhyBase;
class LteRealisticChannelModel;
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

/// Traffic Flow Template ID constants
constexpr TrafficFlowTemplateId TFT_REMOVED_DESTINATION = -2;    // Destination removed from simulation
constexpr TrafficFlowTemplateId TFT_EXTERNAL_DESTINATION = -1;   // External destination (gateway)
constexpr TrafficFlowTemplateId TFT_LOCAL_DELIVERY = 0;          // Local delivery (same base station)
constexpr TrafficFlowTemplateId TFT_MEC_HOST = -3;               // MEC host destination

// Attenuation vector for analogue models
typedef std::vector<double> AttenuationVector;


/*************************
*   Transmission Modes  *
*************************/

/// Number of transmission modes in DL direction.
const unsigned char DL_NUM_TXMODE = MULTI_USER + 1;

/// Number of transmission modes in UL direction.
const unsigned char UL_NUM_TXMODE = MULTI_USER + 1;


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
    std::string str() const;
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
void verifyControlInfo(const FlowControlInfo *info);
bool isNrUe(MacNodeId id);
FeedbackType getFeedbackType(std::string s);
RbAllocationType getRbAllocationType(std::string s);
const std::string lteTrafficClassToA(LteTrafficClass type);
LteTrafficClass aToLteTrafficClass(std::string s);
const std::string phyFrameTypeToA(const LtePhyFrameType r);
LtePhyFrameType aToPhyFrameType(std::string s);
const std::string rlcTypeToA(LteRlcType type);
char *cStringToLower(char *str);
LteRlcType aToRlcType(std::string s);
const std::string planeToA(Plane p);
MacNodeId ctrlInfoToUeId(const FlowControlInfo *info);
MacCid ctrlInfoToMacCid(const FlowControlInfo *info);        // get the CID from the packet control info
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

} //namespace

#endif
