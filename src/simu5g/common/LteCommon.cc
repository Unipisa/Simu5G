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

#include "simu5g/common/LteCommon.h"

#include <inet/common/packet/dissector/ProtocolDissectorRegistry.h>
#include <inet/networklayer/ipv4/Ipv4ProtocolDissector.h>
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

const inet::Protocol LteProtocol::ipv4uu("ipv4uu", "IPv4 (LTE Uu link)");
Register_Protocol_Dissector(&LteProtocol::ipv4uu, Ipv4ProtocolDissector);

const inet::Protocol LteProtocol::pdcp("pdcp", "PDCP");         // Packet Data Convergence Protocol
const inet::Protocol LteProtocol::rlc("rlc", "RLC");            // Radio Link Control
const inet::Protocol LteProtocol::ltemac("ltemac", "LTE-MAC");  // Medium Access Control
const inet::Protocol LteProtocol::gtp("gtp", "GTP");            // GPRS Tunneling Protocol
const inet::Protocol LteProtocol::x2ap("x2ap", "X2AP");         // X2AP Protocol

const std::string lteTrafficClassToA(LteTrafficClass type)
{
    switch (type) {
        case CONVERSATIONAL:
            return "CONVERSATIONAL";
        case STREAMING:
            return "STREAMING";
        case INTERACTIVE:
            return "INTERACTIVE";
        case BACKGROUND:
            return "BACKGROUND";
        default:
            return "UNKNOWN_TRAFFIC_TYPE";
    }
}

LteTrafficClass aToLteTrafficClass(std::string s)
{
    if (s == "CONVERSATIONAL")
        return CONVERSATIONAL;
    if (s == "STREAMING")
        return STREAMING;
    if (s == "INTERACTIVE")
        return INTERACTIVE;
    if (s == "BACKGROUND")
        return BACKGROUND;
    return UNKNOWN_TRAFFIC_TYPE;
}

const std::string rlcTypeToA(LteRlcType type)
{
    switch (type) {
        case TM:
            return "tm";
        case UM:
            return "um";
        case AM:
            return "am";
        default:
            return "UNKNOWN_RLC_TYPE";
    }
}

char *cStringToLower(char *str)
{
    for (int i = 0; str[i]; i++)
        str[i] = tolower(str[i]);

    return str;
}

LteRlcType aToRlcType(std::string s)
{
    if (s == "TM")
        return TM;
    if (s == "UM")
        return UM;
    if (s == "AM")
        return AM;
    return UNKNOWN_RLC_TYPE;
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

FeedbackGeneratorType getFeedbackGeneratorType(std::string s)
{
    if (s == "IDEAL")
        return IDEAL;
    if (s == "REAL")
        return REAL;
    if (s == "DAS_AWARE")
        return DAS_AWARE;
    //default
    return IDEAL;
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

const std::string nodeTypeToA(const RanNodeType t)
{
    const char * str = omnetpp::cEnum::get("simu5g::RanNodeType")->getStringFor((intval_t)t);
    return str ? str : "UNKNOWN_NODE_TYPE";
}

RanNodeType aToNodeType(std::string name)
{
    return static_cast<RanNodeType>(omnetpp::cEnum::get("simu5g::RanNodeType")->lookup(name.c_str(), UNKNOWN_NODE_TYPE));
}

const std::string applicationTypeToA(ApplicationType a)
{
    const char * str = omnetpp::cEnum::get("simu5g::ApplicationType")->getStringFor((intval_t)a);
    return str ? str : "UNKNOWN_APP";
}

ApplicationType aToApplicationType(std::string s)
{
    return static_cast<ApplicationType>(omnetpp::cEnum::get("simu5g::ApplicationType")->lookup(s.c_str(), UNKNOWN_APP));
}

const std::string fbGeneratorTypeToA(FeedbackGeneratorType type)
{
    const char * str = omnetpp::cEnum::get("simu5g::FeedbackGeneratorType")->getStringFor((intval_t)type);
    return str ? str : "UNKNOW_FB_GEN_TYPE";
}

RanNodeType getNodeTypeById(MacNodeId id)
{
    if (id >= ENB_MIN_ID && id <= ENB_MAX_ID)
        return ENODEB;
    if (id >= UE_MIN_ID && id <= UE_MAX_ID)
        return UE;
    return UNKNOWN_NODE_TYPE;
}

void verifyControlInfo(const FlowControlInfo *info)
{
    auto srcType = getNodeTypeById(info->getSourceId());
    auto destType = getNodeTypeById(info->getDestId());
    bool isMulticast = info->getMulticastGroupId() != -1;

    switch ((Direction)info->getDirection()) {
        case UL:
            ASSERT(!isMulticast);
            ASSERT(srcType == UE);
            ASSERT(destType == ENODEB);
            break;
        case DL:
            ASSERT(!isMulticast);
            ASSERT(srcType == ENODEB);
            ASSERT(destType == UE);
            break;
        case D2D:
            ASSERT(!isMulticast);
            ASSERT(srcType == UE);
            ASSERT(destType == UE);
            break;
        case D2D_MULTI:
            ASSERT(isMulticast);
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
    return getNodeTypeById(id) == UE && id >= NR_UE_MIN_ID;
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

const std::string DeploymentScenarioToA(DeploymentScenario type)
{
    const char * str = omnetpp::cEnum::get("simu5g::DeploymentScenario")->getStringFor((intval_t)type);
    return str ? str : "UNKNOW_SCENARIO";
}

DeploymentScenario aToDeploymentScenario(std::string s)
{
    return static_cast<DeploymentScenario>(omnetpp::cEnum::get("simu5g::DeploymentScenario")->lookup(s.c_str(), UNKNOW_SCENARIO));
}

bool isMulticastConnection(LteControlInfo *lteInfo)
{
    return lteInfo->getMulticastGroupId() >= 0;
}


/*
 * Obtain the CID from the Control Info
 */
MacCid ctrlInfoToMacCid(const FlowControlInfo *info)
{
    return MacCid(ctrlInfoToUeId(info), info->getLcid());
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
        case UL: case D2D_MULTI: // D2D_MULTI goes here, since the destination id is meaningless in that context
            return info->getSourceId();
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
        << ", type=" << nodeTypeToA(nodeType) << "/" << (type == MACRO_ENB ? "MACRO" : "MICRO")
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
