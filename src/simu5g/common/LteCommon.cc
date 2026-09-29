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

#include "simu5g/common/LteCommon.h"

#include <inet/common/packet/PacketFilter.h>
#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/common/ProtocolTag_m.h>

#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/cellInfo/CellInfo.h"
#include "simu5g/corenetwork/trafficFlowFilter/TftControlInfo_m.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/x2/packet/X2ControlInfo_m.h"

namespace simu5g {

using namespace omnetpp;

using namespace inet;

const inet::Protocol LteProtocol::pdcp("pdcp", "PDCP");         // Packet Data Convergence Protocol
const inet::Protocol LteProtocol::rlc("rlc", "RLC");            // Radio Link Control
const inet::Protocol LteProtocol::ltemac("ltemac", "LTE-MAC");  // Medium Access Control
const inet::Protocol LteProtocol::gtp("gtp", "GTP");            // GPRS Tunneling Protocol
const inet::Protocol LteProtocol::x2ap("x2ap", "X2AP");         // X2AP Protocol
const inet::Protocol LteProtocol::sdap("sdap", "SDAP");         // Service Data Adaptation Protocol
const inet::Protocol LteProtocol::unstructured("unstructured", "Unstructured");   // The payload of an Unstructured session

const std::string rlcModeToA(RlcMode type)
{
    switch (type) {
        case TM:
            return "tm";
        case UM:
            return "um";
        case AM:
            return "am";
        default:
            return "UNKNOWN_RLC_MODE";
    }
}

char *cStringToLower(char *str)
{
    for (int i = 0; str[i]; i++)
        str[i] = tolower(str[i]);

    return str;
}

RlcMode aToRlcMode(std::string s)
{
    if (s == "TM")
        return TM;
    if (s == "UM")
        return UM;
    if (s == "AM")
        return AM;
    return UNKNOWN_RLC_MODE;
}

const std::string sessionTypeToA(SessionType type)
{
    switch (type) {
        case IP_V4:
            return "IPv4";
        case IP_V6:
            return "IPv6";
        case IP_V4V6:
            return "IPv4v6";
        case ETHERNET:
            return "Ethernet";
        case UNSTRUCTURED:
            return "Unstructured";
        default:
            return "UNKNOWN_SESSION_TYPE";
    }
}

SessionType aToSessionType(std::string s)
{
    if (s == "IPv4")
        return IP_V4;
    if (s == "IPv6")
        return IP_V6;
    if (s == "IPv4v6")
        return IP_V4V6;
    if (s == "Ethernet")
        return ETHERNET;
    if (s == "Unstructured")
        return UNSTRUCTURED;
    throw cRuntimeError("Unknown session type '%s' (expected \"IPv4\", \"IPv6\", \"IPv4v6\", \"Ethernet\" or \"Unstructured\")", s.c_str());
}

bool isIpSessionType(SessionType type)
{
    return type == IP_V4 || type == IP_V6 || type == IP_V4V6;
}

const std::string coreNetworkToA(CoreNetwork type)
{
    switch (type) {
        case CN_EPC:
            return "epc";
        case CN_5GC:
            return "5gc";
        default:
            return "UNKNOWN_CORE_NETWORK";
    }
}

CoreNetwork aToCoreNetwork(std::string s)
{
    if (s == "epc")
        return CN_EPC;
    if (s == "5gc")
        return CN_5GC;
    return UNKNOWN_CORE_NETWORK;
}

const std::string cellGroupToA(CellGroup group)
{
    switch (group) {
        case MCG:
            return "MCG";
        case SCG:
            return "SCG";
        default:
            return "UNKNOWN_CELL_GROUP";
    }
}

CellGroup aToCellGroup(std::string s)
{
    if (s == "MCG")
        return MCG;
    if (s == "SCG")
        return SCG;
    return UNKNOWN_CELL_GROUP;
}

void configurePacketFilter(inet::PacketFilter& filter, const char *spec)
{
    std::string s = spec;
    if (s.rfind("expr(", 0) == 0 && !s.empty() && s.back() == ')')
        filter.setExpression(s.substr(5, s.size() - 6).c_str());
    else
        filter.setPattern(spec);
}

cDynamicExpression *getExpressionFromPar(cPar& par, cDynamicExpression::IResolver *resolver)
{
    cObject *obj = par.objectValue();
    auto *exprObj = dynamic_cast<cOwnedDynamicExpression *>(obj);
    if (!exprObj)
        throw cRuntimeError("Parameter '%s' must be an expr() expression", par.getFullPath().c_str());
    auto *expr = exprObj->dup();
    expr->setResolver(resolver);
    return expr;
}

const std::string dirToA(Direction dir)
{
    switch (dir) {
        case DL:
            return "DL";
        case UL:
            return "UL";
        case D2D:
            return "D2D";
        case D2D_MULTI:
            return "D2D_MULTI";
        default:
            return "Unrecognized";
    }
}

Direction directionFromBsrLcid(LogicalCid lcid, Direction fallback)
{
    if (lcid == D2D_SHORT_BSR)
        return D2D;
    if (lcid == D2D_MULTI_SHORT_BSR)
        return D2D_MULTI;
    return fallback;
}

const std::string d2dModeToA(LteD2DMode mode)
{
    switch (mode) {
        case IM:
            return "IM";
        case DM:
            return "DM";
        default:
            return "Unrecognized";
    }
}

const std::string allocationTypeToA(RbAllocationType type)
{
    switch (type) {
        case TYPE2_DISTRIBUTED:
            return "distributed";
        case TYPE2_LOCALIZED:
            return "localized";
    }
    return "Unrecognized";
}

const std::string modToA(LteMod mod)
{
    switch (mod) {
        case _QPSK:
            return "QPSK";
        case _16QAM:
            return "16-QAM";
        case _64QAM:
            return "64-QAM";
        case _256QAM:
            return "256-QAM";
    }
    return "UNKNOWN";
}

const std::string periodicityToA(FbPeriodicity per)
{
    switch (per) {
        case PERIODIC:
            return "PERIODIC";
        case APERIODIC:
            return "APERIODIC";
    }
    return "UNKNOWN";
}

FeedbackType getFeedbackType(std::string s)
{
    if (s == "ALLBANDS")
        return ALLBANDS;
    if (s == "PREFERRED")
        return PREFERRED;
    if (s == "WIDEBAND")
        return WIDEBAND;

    return WIDEBAND; // default
}

RbAllocationType getRbAllocationType(std::string s)
{
    if (s == "distributed")
        return TYPE2_DISTRIBUTED;
    if (s == "localized")
        return TYPE2_LOCALIZED;

    return TYPE2_DISTRIBUTED; // default type
}

const std::string txModeToA(TxMode tx)
{
    const char * str = omnetpp::cEnum::get("simu5g::TxMode")->getStringFor((intval_t)tx);
    return str ? str : "UNKNOWN_TX_MODE";
}

TxMode aToTxMode(std::string s)
{
    return static_cast<TxMode>(omnetpp::cEnum::get("simu5g::TxMode")->lookup(s.c_str(), UNKNOWN_TX_MODE));
}

const std::string schedDisciplineToA(SchedDiscipline discipline)
{
    const char * str = omnetpp::cEnum::get("simu5g::SchedDiscipline")->getStringFor((intval_t)discipline);
    return str ? str : "UNKNOWN_DISCIPLINE";
}

SchedDiscipline aToSchedDiscipline(std::string s)
{
    return static_cast<SchedDiscipline>(omnetpp::cEnum::get("simu5g::SchedDiscipline")->lookup(s.c_str(), UNKNOWN_DISCIPLINE));
}

const std::string dasToA(const Remote r)
{
    const char * str = omnetpp::cEnum::get("simu5g::Remote")->getStringFor((intval_t)r);
    return str ? str : "UNKNOWN_RU";
}

Remote aToDas(std::string s)
{
    return static_cast<Remote>(omnetpp::cEnum::get("simu5g::Remote")->lookup(s.c_str(), UNKNOWN_RU));
}

const std::string phyFrameTypeToA(const LtePhyFrameType r)
{
    const char * str = omnetpp::cEnum::get("simu5g::LtePhyFrameType")->getStringFor((intval_t)r);
    return str ? str : "UNKNOWN_TYPE";
}

LtePhyFrameType aToPhyFrameType(std::string s)
{
    return static_cast<LtePhyFrameType>(omnetpp::cEnum::get("simu5g::LtePhyFrameType")->lookup(s.c_str(), UNKNOWN_TYPE));
}

const char *nodeTypeToA(RanNodeType t)
{
    switch (t) {
        case UE: return "UE";
        case NODEB: return "NODEB";
        default: return "UNKNOWN";
    }
}

RanNodeType aToNodeType(std::string name)
{
    if (name == "UE")
        return UE;
    else if (name == "ENODEB" || name == "GNODEB")
        return NODEB;
    else
        throw cRuntimeError("Unknown node type: %s", name.c_str());
}

RanNodeType getNodeTypeById(MacNodeId id)
{
    if (num(id) >= ENB_MIN_ID && num(id) <= ENB_MAX_ID)
        return NODEB;
    if (num(id) >= UE_MIN_ID && num(id) <= UE_MAX_ID)
        return UE;
    return UNKNOWN_NODE_TYPE;
}

void verifyControlInfo(const FlowControlInfo *info)
{
    auto srcType = getNodeTypeById(info->getSourceId());
    auto destType = getNodeTypeById(info->getDestId());
    bool isGroupcast = info->getD2dGroupId() != NODEID_NONE;

    switch (info->getDirection()) {
        case UL:
            ASSERT(!isGroupcast);
            ASSERT(srcType == UE);
            ASSERT(destType == NODEB);
            break;
        case DL:
            ASSERT(!isGroupcast);
            ASSERT(srcType == NODEB);
            ASSERT(destType == UE);
            break;
        case D2D:
            ASSERT(!isGroupcast);
            ASSERT(srcType == UE);
            ASSERT(destType == UE);
            break;
        case D2D_MULTI:
            ASSERT(isGroupcast);
            ASSERT(srcType == UE);
            //ASSERT(destType == UE);
            break;
        default:
            throw cRuntimeError("Unknown direction %d", info->getDirection());
    }
}

bool isBaseStation(CoreNodeType nodeType)
{
    return nodeType == ENB || nodeType == GNB;
}

bool isNrUe(MacNodeId id)
{
    return getNodeTypeById(id) == UE && num(id) >= NR_UE_MIN_ID;
}

const std::string planeToA(Plane p)
{
    switch (p) {
        case MAIN_PLANE:
            return "MAIN_PLANE";
        case MU_MIMO_PLANE:
            return "MU_MIMO_PLANE";
        default:
            return "UNKNOWN PLANE";
    }
}

/*
 * Obtain the DrbKey for TX context (key by destination, i.e. remote receiver)
 */
DrbKey FlowId::txDrbKey() const
{
    if (direction == D2D_MULTI) {
        ASSERT(d2dGroupId != NODEID_NONE);
        return DrbKey(d2dGroupId, drbId);
    }
    return DrbKey(destId, drbId);
}

/*
 * Obtain the DrbKey for RX context (key by source, i.e. remote sender)
 */
DrbKey FlowId::rxDrbKey() const
{
    return DrbKey(sourceId, drbId);
}

// Build the FlowId of the reverse leg of a duplex bearer: same DRB, endpoints and D2D
// peer roles swapped, direction reversed (UL<->DL; a D2D flow's reverse is D2D between
// the same pair).
FlowId FlowId::reversed() const
{
    ASSERT(direction != D2D_MULTI); // multicast bearers stay unidirectional

    FlowId rev = *this;
    rev.sourceId = destId;
    rev.destId = sourceId;
    rev.direction = (direction == UL) ? DL :
                    (direction == DL) ? UL : direction;
    rev.d2dTxPeerId = d2dRxPeerId;
    rev.d2dRxPeerId = d2dTxPeerId;
    return rev;
}

/*
 * Obtain the DrbKey for TX context (key by destination, i.e. remote receiver)
 */
DrbKey ctrlInfoToTxDrbKey(const FlowControlInfo *info)
{
    if (info->getDirection() == D2D_MULTI) {
        ASSERT(info->getD2dGroupId() != NODEID_NONE);
        return DrbKey(info->getD2dGroupId(), info->getDrbId());
    }
    return DrbKey(info->getDestId(), info->getDrbId());
}

/*
 * Obtain the DrbKey for RX context (key by source, i.e. remote sender).
 *
 * For D2D_MULTI, we use sourceId (not d2dGroupId) because multiple
 * senders can relay to the same multicast group, and each needs its own
 * RX buffer. Using d2dGroupId would cause key collisions across
 * senders. In theory, (sourceId, drbId) could collide if the same sender
 * participates in multiple multicast groups that share the same drbId,
 * but in practice each group uses a distinct drbId so this does not occur.
 * A fully correct solution would require extending DrbKey to three fields
 * (sourceId, d2dGroupId, drbId).
 */
DrbKey ctrlInfoToRxDrbKey(const FlowControlInfo *info)
{
    return DrbKey(info->getSourceId(), info->getDrbId());
}

const std::string DeploymentScenarioToA(DeploymentScenario type)
{
    const char * str = omnetpp::cEnum::get("simu5g::DeploymentScenario")->getStringFor((intval_t)type);
    return str ? str : "UNKNOWN_SCENARIO";
}

DeploymentScenario aToDeploymentScenario(std::string s)
{
    return static_cast<DeploymentScenario>(omnetpp::cEnum::get("simu5g::DeploymentScenario")->lookup(s.c_str(), UNKNOWN_SCENARIO));
}


/*
 * Obtain the MacNodeId of a UE from packet control info
 */
MacNodeId ctrlInfoToUeId(const FlowControlInfo *info)
{
    /*
     * direction | src       dest
     * ---------------------------
     *    DL     | eNb ---->  UE
     *    UL     | UE  ---->  eNb
     *    D2D    | UE  ---->  UE
     *
     */
    switch (info->getDirection()) {
        case DL: case D2D:
            return info->getDestId();
        case UL:
            return info->getSourceId();
        case D2D_MULTI:
            ASSERT(info->getD2dGroupId() != NODEID_NONE);
            return info->getD2dGroupId();
        default:
            throw cRuntimeError("ctrlInfoToMacCid - unknown direction %d", info->getDirection());
    }
}



void getParametersFromXML(cXMLElement *xmlData, ParameterMap& outputMap)
{
    cXMLElementList parameters = xmlData->getElementsByTagName("Parameter");

    for (auto parameter : parameters) {
        const char *name = parameter->getAttribute("name");
        const char *type = parameter->getAttribute("type");
        const char *value = parameter->getAttribute("value");
        if (name == nullptr || type == nullptr || value == nullptr) {
            EV << "Invalid parameter, could not find name, type or value." << endl;
            continue;
        }

        std::string sType = type;     // needed for easier comparison
        std::string sValue = value;    // needed for easier comparison

        cMsgPar param(name);

        // parse type of parameter and set value
        if (sType == "bool") {
            param.setBoolValue(sValue == "true" || sValue == "1");
        }
        else if (sType == "double") {
            param.setDoubleValue(strtod(value, nullptr));
        }
        else if (sType == "string") {
            param.setStringValue(value);
        }
        else if (sType == "long") {
            param.setLongValue(strtol(value, nullptr, 0));
        }
        else {
            EV << "Unknown parameter type: \"" << sType << "\"" << endl;
            continue;
        }

        // add parameter to output map
        outputMap[name] = param;
    }
}

void parseStringToIntArray(std::string str, int *values, int dim, int pad)
{
    for (int i = 0; i < dim; i++) {
        int pos = str.find(',');

        if (pos == -1) {
            // last number or malformed string
            pos = str.find(';');
            if (pos == -1) {
                // malformed string or too few numbers, use 0 for remaining entries
                for (int j = i; j < dim; j++) {
                    values[j] = pad;
                }
                EV << "parseStringToIntArray: Error: too few values in string array, padding with " << pad << endl;
                break;
            }
        }
        std::string num = str.substr(0, pos);
        std::istringstream ss(num);
        ss >> values[i];
        str = str.erase(0, pos + 1);
    }

    if (!str.empty())
        throw cRuntimeError("parseStringToIntArray(): more values in string than nodes");
}

double linearToDBm(double linear)
{
    return 10 * log10(1000 * linear);
}

double linearToDb(double linear)
{
    return 10 * log10(linear);
}

double dBmToLinear(double db)
{
    return pow(10, (db - 30) / 10);
}

double dBToLinear(double db)
{
    return pow(10, (db) / 10);
}

void initializeAllChannels(cModule *mod)
{
    for (cModule::GateIterator i(mod); !i.end(); i++) {
        cGate *gate = *i;
        if (gate->getChannel() != nullptr) {
            if (!gate->getChannel()->initialized()) {
                gate->getChannel()->callInitialize();
            }
        }
    }

    for (cModule::SubmoduleIterator i(mod); !i.end(); i++) {
        cModule *submodule = *i;
        initializeAllChannels(submodule);
    }
}

void removeAllSimu5GTags(inet::Packet *pkt)
{
    pkt->removeTagIfPresent<TftControlInfo>();
    pkt->removeTagIfPresent<X2ControlInfoTag>();
    pkt->removeTagIfPresent<FlowControlInfo>();
    pkt->removeTagIfPresent<UserControlInfo>();
    pkt->removeTagIfPresent<LteControlInfo>();
}

std::string EnbInfo::str() const
{
    std::ostringstream oss;
    oss << "EnbInfo[id=" << id << ", module=" << (eNodeB ? eNodeB->getFullName() : "null")
        << ", type=" << (isNr ? "NR" : "LTE") << "/" << (type == MACRO_ENB ? "MACRO" : "MICRO")
        << ", init=" << (init ? "Y" : "N") << ", txPwr=" << txPwr << "dBm]";
    return oss.str();
}

std::string UeInfo::str() const
{
    std::ostringstream oss;
    oss << "UeInfo[id=" << id << ", module=" << (ue ? ue->getFullName() : "null")
        << ", cellId=" << cellId << ", init=" << (init ? "Y" : "N")
        << ", txPwr=" << txPwr << "dBm]";
    return oss.str();
}

std::string BgTrafficManagerInfo::str() const
{
    std::ostringstream oss;
    oss << "BgTrafficManagerInfo[freq=" << carrierFrequency << "GHz"
        << ", init=" << (init ? "Y" : "N") << ", rbsDL=" << allocatedRbsDl
        << ", rbsUL=" << allocatedRbsUl << ", numUEs=" << allocatedRbsUeUl.size() << "]";
    return oss.str();
}

} //namespace
