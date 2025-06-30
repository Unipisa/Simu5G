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

#include "common/LteCommon.h"

#include <inet/common/packet/dissector/ProtocolDissectorRegistry.h>
#include <inet/networklayer/ipv4/Ipv4ProtocolDissector.h>
#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/common/ProtocolTag_m.h>

#include "common/LteControlInfo.h"
#include "common/binder/Binder.h"
#include "common/cellInfo/CellInfo.h"
#include "corenetwork/trafficFlowFilter/TftControlInfo_m.h"
#include "stack/mac/LteMacEnb.h"
#include "x2/packet/X2ControlInfo_m.h"

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
    int i = 0;
    while (txmodes[i].tx != UNKNOWN_TX_MODE) {
        if (txmodes[i].tx == tx)
            return txmodes[i].txName;
        i++;
    }
    return "UNKNOWN_TX_MODE";
}

TxMode aToTxMode(std::string s)
{
    int i = 0;
    while (txmodes[i].tx != UNKNOWN_TX_MODE) {
        if (txmodes[i].txName == s)
            return txmodes[i].tx;
        i++;
    }
    return UNKNOWN_TX_MODE;
}

const std::string schedDisciplineToA(SchedDiscipline discipline)
{
    int i = 0;
    while (disciplines[i].discipline != UNKNOWN_DISCIPLINE) {
        if (disciplines[i].discipline == discipline)
            return disciplines[i].disciplineName;
        i++;
    }
    return "UNKNOWN_DISCIPLINE";
}

SchedDiscipline aToSchedDiscipline(std::string s)
{
    int i = 0;
    while (disciplines[i].discipline != UNKNOWN_DISCIPLINE) {
        if (disciplines[i].disciplineName == s)
            return disciplines[i].discipline;
        i++;
    }
    return UNKNOWN_DISCIPLINE;
}

const std::string dasToA(const Remote r)
{
    int i = 0;
    while (remotes[i].remote != UNKNOWN_RU) {
        if (remotes[i].remote == r)
            return remotes[i].remoteName;
        i++;
    }
    return "UNKNOWN_RU";
}

Remote aToDas(std::string s)
{
    int i = 0;
    while (remotes[i].remote != UNKNOWN_RU) {
        if (remotes[i].remoteName == s)
            return remotes[i].remote;
        i++;
    }
    return UNKNOWN_RU;
}

const std::string phyFrameTypeToA(const LtePhyFrameType r)
{
    int i = 0;
    while (phytypes[i].phyType != UNKNOWN_TYPE) {
        if (phytypes[i].phyType == r)
            return phytypes[i].phyName;
        i++;
    }
    return "UNKNOWN_TYPE";
}

LtePhyFrameType aToPhyFrameType(std::string s)
{
    int i = 0;
    while (phytypes[i].phyType != UNKNOWN_TYPE) {
        if (phytypes[i].phyName == s)
            return phytypes[i].phyType;
        i++;
    }
    return UNKNOWN_TYPE;
}

const std::string nodeTypeToA(const RanNodeType t)
{
    int i = 0;
    while (nodetypes[i].node != UNKNOWN_NODE_TYPE) {
        if (nodetypes[i].node == t)
            return nodetypes[i].nodeName;
        i++;
    }
    return "UNKNOWN_NODE_TYPE";
}

RanNodeType aToNodeType(std::string name)
{
    int i = 0;
    while (nodetypes[i].node != UNKNOWN_NODE_TYPE) {
        if (nodetypes[i].nodeName == name)
            return nodetypes[i].node;
        i++;
    }
    return UNKNOWN_NODE_TYPE;
}

const std::string applicationTypeToA(ApplicationType a)
{
    int i = 0;
    while (applications[i].app != UNKNOWN_APP) {
        if (applications[i].app == a)
            return applications[i].appName;
        i++;
    }
    return "UNKNOWN_APP";
}

ApplicationType aToApplicationType(std::string s)
{
    int i = 0;
    while (applications[i].app != UNKNOWN_APP) {
        if (applications[i].appName == s)
            return applications[i].app;
        i++;
    }
    return UNKNOWN_APP;
}

const std::string fbGeneratorTypeToA(FeedbackGeneratorType type)
{
    int i = 0;
    while (feedbackGeneratorTypeTable[i].ty != UNKNOW_FB_GEN_TYPE) {
        if (feedbackGeneratorTypeTable[i].ty == type)
            return feedbackGeneratorTypeTable[i].tyname;
        i++;
    }
    return "UNKNOW_FB_GEN_TYPE";
}

RanNodeType getNodeTypeById(MacNodeId id)
{
    if (id >= ENB_MIN_ID && id <= ENB_MAX_ID)
        return ENODEB;
    if (id >= UE_MIN_ID && id <= UE_MAX_ID)
        return UE;
    return UNKNOWN_NODE_TYPE;
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
    int i = 0;
    while (DeploymentScenarioTable[i].scenario != UNKNOW_SCENARIO) {
        if (DeploymentScenarioTable[i].scenario == type)
            return DeploymentScenarioTable[i].scenarioName;
        i++;
    }
    return "UNKNOW_SCENARIO";
}

DeploymentScenario aToDeploymentScenario(std::string s)
{
    int i = 0;
    while (DeploymentScenarioTable[i].scenario != UNKNOW_SCENARIO) {
        if (DeploymentScenarioTable[i].scenarioName == s)
            return DeploymentScenarioTable[i].scenario;
        i++;
    }
    return UNKNOW_SCENARIO;
}

bool isMulticastConnection(LteControlInfo *lteInfo)
{
    return lteInfo->getMulticastGroupId() >= 0;
}

MacCid idToMacCid(MacNodeId nodeId, LogicalCid lcid)
{
    return ((MacCid)nodeId << 16) | lcid;
}

/*
 * Obtain the CID from the Control Info
 */
MacCid ctrlInfoToMacCid(inet::Ptr<LteControlInfo> info)
{
    /*
     * Given the fact that  CID = <UE_MAC_ID,LCID>
     * we choose the proper MAC ID based on the direction of the packet
     *
     * direction | src       dest
     * ---------------------------
     *    DL     | eNb ---->  UE
     *    UL     | UE  ---->  eNb
     *    D2D    | UE  ---->  UE
     *
     */
    unsigned int dir = info->getDirection();
    LogicalCid lcid = info->getLcid();
    MacNodeId ueId;

    switch (dir) {
        case DL: case D2D:
            ueId = info->getDestId();
            break;
        case UL: case D2D_MULTI:  // D2D_MULTI goes here, since the destination id is meaningless in that context
            ueId = info->getSourceId();
            break;
        default:
            throw cRuntimeError("ctrlInfoToMacCid(): unknown direction %d", dir);
    }
    EV << "ctrlInfoToMacCid - dir[" << dir << "] - ueId[" << ueId << "] - lcid[" << lcid << "]" << endl;
    return idToMacCid(ueId, lcid);
}

/*
 * Obtain the MacNodeId of a UE from packet control info
 */
MacNodeId ctrlInfoToUeId(inet::Ptr<LteControlInfo> info)
{
    /*
     * direction | src       dest
     * ---------------------------
     *    DL     | eNb ---->  UE
     *    UL     | UE  ---->  eNb
     *    D2D    | UE  ---->  UE
     *
     */
    unsigned int dir = info->getDirection();
    MacNodeId ueId;

    switch (dir) {
        case DL: case D2D:
            ueId = info->getDestId();
            break;
        case UL: case D2D_MULTI: // D2D_MULTI goes here, since the destination id is meaningless in that context
            ueId = info->getSourceId();
            break;
        default:
            throw cRuntimeError("ctrlInfoToMacCid - unknown direction %d", dir);
    }
    return ueId;
}

MacNodeId MacCidToNodeId(MacCid cid)
{
    return (MacNodeId)(cid >> 16);
}

LogicalCid MacCidToLcid(MacCid cid)
{
    return (LogicalCid)(cid);
}

CellInfo *getCellInfo(Binder *binder, MacNodeId nodeId)
{
    // Check if it is an eNodeB
    // function GetNextHop returns nodeId
    MacNodeId id = binder->getNextHop(nodeId);
    OmnetId omnetid = binder->getOmnetId(id);
    cModule *module = getSimulation()->getModule(omnetid);
    return module ? check_and_cast<CellInfo *>(module->getSubmodule("cellInfo")) : nullptr;
}

cModule *getPhyByMacNodeId(Binder *binder, MacNodeId nodeId)
{
    // UE might have left the simulation, return NULL in this case
    // since we do not have a MAC-Module anymore
    int id = binder->getOmnetId(nodeId);
    if (id == 0) {
        return nullptr;
    }
    if (isNrUe(nodeId))
        return getSimulation()->getModule(id)->getSubmodule("cellularNic")->getSubmodule("nrPhy");
    return getSimulation()->getModule(id)->getSubmodule("cellularNic")->getSubmodule("phy");
}

cModule *getMacByMacNodeId(Binder *binder, MacNodeId nodeId)
{
    // UE might have left the simulation, return NULL in this case
    // since we do not have a MAC-Module anymore
    int id = binder->getOmnetId(nodeId);
    if (id == 0) {
        return nullptr;
    }
    if (isNrUe(nodeId))
        return getSimulation()->getModule(id)->getSubmodule("cellularNic")->getSubmodule("nrMac");
    return getSimulation()->getModule(id)->getSubmodule("cellularNic")->getSubmodule("mac");
}

cModule *getRlcByMacNodeId(Binder *binder, MacNodeId nodeId, LteRlcType rlcType)
{
    cModule *module = getMacByMacNodeId(binder, nodeId);
    if (module == nullptr) {
        return nullptr;
    }
    if (isNrUe(nodeId))
        return getSimulation()->getModule(binder->getOmnetId(nodeId))->getSubmodule("cellularNic")->getSubmodule("nrRlc")->getSubmodule(rlcTypeToA(rlcType).c_str());
    return getSimulation()->getModule(binder->getOmnetId(nodeId))->getSubmodule("cellularNic")->getSubmodule("rlc")->getSubmodule(rlcTypeToA(rlcType).c_str());
}

cModule *getPdcpByMacNodeId(Binder *binder, MacNodeId nodeId)
{
    // UE might have left the simulation, return NULL in this case
    // since we do not have a MAC-Module anymore
    int id = binder->getOmnetId(nodeId);
    if (id == 0) {
        return nullptr;
    }
    return getSimulation()->getModule(id)->getSubmodule("cellularNic")->getSubmodule("pdcpRrc");
}

LteMacBase *getMacUe(Binder *binder, MacNodeId nodeId)
{
    return check_and_cast<LteMacBase *>(getMacByMacNodeId(binder, nodeId));
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

} //namespace

