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

#include <algorithm>
#include <cctype>

#include <inet/common/stlutils.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "simu5g/common/binder/Binder.h"
#include "simu5g/corenetwork/statsCollector/BaseStationStatsCollector.h"
#include "simu5g/corenetwork/statsCollector/UeStatsCollector.h"
#include "simu5g/stack/mac/LteMacUe.h"
#include "simu5g/stack/phy/LtePhyUe.h"
#include "simu5g/common/cellInfo/CellInfo.h"

namespace simu5g {

using namespace omnetpp;

using namespace std;
using namespace inet;

Define_Module(Binder);

void Binder::registerCarrier(GHz carrierFrequency, unsigned int carrierNumBands, unsigned int numerologyIndex, bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl)
{
    CarrierInfoMap::iterator it = componentCarriers_.find(carrierFrequency);
    if (it != componentCarriers_.end() && carrierNumBands <= componentCarriers_[carrierFrequency].numBands) {
        EV << "Binder::registerCarrier - Carrier @ " << carrierFrequency << "GHz already registered" << endl;
    }
    else {
        CarrierInfo cInfo;
        cInfo.carrierFrequency = carrierFrequency;
        cInfo.numBands = carrierNumBands;
        cInfo.numerologyIndex = numerologyIndex;
        cInfo.slotFormat = computeSlotFormat(useTdd, tddNumSymbolsDl, tddNumSymbolsUl);
        componentCarriers_[carrierFrequency] = cInfo;

        // update total number of bands in the system
        totalBands_ += carrierNumBands;

        EV << "Binder::registerCarrier - Registered component carrier @ " << carrierFrequency << "GHz" << endl;

        carrierFreqToNumerologyIndex_[carrierFrequency] = numerologyIndex;

        // add new (empty) entry to carrierUeMap
        std::set<MacNodeId> tempSet;
        carrierUeMap_[carrierFrequency] = tempSet;
    }
}

void Binder::registerCarrierUe(GHz carrierFrequency, unsigned int numerologyIndex, MacNodeId ueId)
{
    // check if carrier exists in the system
    CarrierUeMap::iterator it = carrierUeMap_.find(carrierFrequency);
    if (it == carrierUeMap_.end())
        throw cRuntimeError("Binder::registerCarrierUe - Carrier [%fGHz] not found", carrierFrequency.get());

    carrierUeMap_[carrierFrequency].insert(ueId);

    if (ueNumerologyIndex_.find(ueId) == ueNumerologyIndex_.end()) {
        std::set<NumerologyIndex> numerologySet;
        ueNumerologyIndex_[ueId] = numerologySet;
    }
    ueNumerologyIndex_[ueId].insert(numerologyIndex);

    if (ueMaxNumerologyIndex_.size() <= num(ueId)) {
        ueMaxNumerologyIndex_.resize(num(ueId) + 1);
        ueMaxNumerologyIndex_[num(ueId)] = numerologyIndex;
    }
    else
        ueMaxNumerologyIndex_[num(ueId)] = (numerologyIndex > ueMaxNumerologyIndex_[num(ueId)]) ? numerologyIndex : ueMaxNumerologyIndex_[num(ueId)];
}

const UeSet& Binder::getCarrierUeSet(GHz carrierFrequency)
{
    CarrierUeMap::iterator it = carrierUeMap_.find(carrierFrequency);
    if (it == carrierUeMap_.end())
        throw cRuntimeError("Binder::getCarrierUeSet - Carrier [%fGHz] not found", carrierFrequency.get());

    return carrierUeMap_[carrierFrequency];
}

NumerologyIndex Binder::getUeMaxNumerologyIndex(MacNodeId ueId)
{
    return ueMaxNumerologyIndex_[num(ueId)];
}

const std::set<NumerologyIndex> *Binder::getUeNumerologyIndex(MacNodeId ueId)
{
    if (ueNumerologyIndex_.find(ueId) == ueNumerologyIndex_.end())
        return nullptr;
    return &ueNumerologyIndex_[ueId];
}

SlotFormat Binder::computeSlotFormat(bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl)
{
    SlotFormat sf;
    if (!useTdd) {
        sf.tdd = false;

        // these values are not used when TDD is false
        sf.numDlSymbols = 0;
        sf.numUlSymbols = 0;
        sf.numFlexSymbols = 0;
    }
    else {
        unsigned int rbxDl = 7;  // TODO replace with the parameter obtained from NED file once you moved the function to the Component Carrier

        sf.tdd = true;
        unsigned int numSymbols = rbxDl * 2;

        if (tddNumSymbolsDl + tddNumSymbolsUl > numSymbols)
            throw cRuntimeError("Binder::computeSlotFormat - Number of symbols not valid - DL[%d] UL[%d]", tddNumSymbolsDl, tddNumSymbolsUl);

        sf.numDlSymbols = tddNumSymbolsDl;
        sf.numUlSymbols = tddNumSymbolsUl;
        sf.numFlexSymbols = numSymbols - tddNumSymbolsDl - tddNumSymbolsUl;
    }
    return sf;
}

SlotFormat Binder::getSlotFormat(GHz carrierFrequency)
{
    CarrierInfoMap::iterator it = componentCarriers_.find(carrierFrequency);
    if (it == componentCarriers_.end())
        throw cRuntimeError("Binder::getSlotFormat - Carrier [%fGHz] not found", carrierFrequency.get());

    return it->second.slotFormat;
}

MacNodeId Binder::registerNode(cModule *nodeModule, RanNodeType type, MacNodeId masterId, bool isNr)
{
    Enter_Method_Silent("registerNode");

    ASSERT(type == UE || type == ENODEB || type == GNODEB);
    MacNodeId nodeId = type == UE ?
            MacNodeId(isNr ? macNodeIdCounterNrUe_++ : macNodeIdCounterUe_++) :
            MacNodeId(macNodeIdCounterEnb_++);  // eNB/gNB

    EV << "Binder : Assigning to module " << nodeModule->getName()
       << " with module id " << nodeModule->getId() << " and MacNodeId " << nodeId
       << "\n";

    // registering new node
    NodeInfo nodeInfo(nodeModule);
    nodeInfoMap_[nodeId] = nodeInfo;

    nodeModule->par(isNr ? "nrMacNodeId" : "macNodeId") = num(nodeId);

    if (type == UE) {
        registerServingNode(masterId, nodeId);
    }
    else if (type == ENODEB || type == GNODEB) {
        nodeModule->par("macCellId") = num(nodeId);
        registerMasterNode(masterId, nodeId);  // note: even if masterId == NODEID_NONE!
    }
    return nodeId;
}

void Binder::unregisterNode(MacNodeId id)
{
    EV << NOW << " Binder::unregisterNode - unregistering node " << id << endl;

    for (auto it = ipAddressToMacNodeId_.begin(); it != ipAddressToMacNodeId_.end(); ) {
        if (it->second == id) {
            it = ipAddressToMacNodeId_.erase(it);
        }
        else {
            it++;
        }
    }

    // iterate all nodeIds and find HarqRx buffers dependent on 'id'
    for (const auto& [nodeId, nodeInfo] : nodeInfoMap_) {
        LteMacBase *mac = getMacFromMacNodeId(nodeId);
        mac->unregisterHarqBufferRx(id);
    }

    // remove 'id' from consolidated node info map
    if (nodeInfoMap_.erase(id) != 1) {
        throw cRuntimeError("Cannot unregister node - node id %d - not found", num(id));
    }
    // remove 'id' from ulTransmissionMap_ if currently scheduled
    for (auto& carrier : ulTransmissionMap_) { // all carrier frequency
        for (auto& bands : carrier.second) { // all RB's for current and last TTI (vector<vector<vector<UeAllocationInfo>>>)
            for (auto& ues : bands) { // all Ue's in each block
                for(auto itr = ues.begin(); itr != ues.end(); ) {
                    if (itr->nodeId == id) {
                        itr = ues.erase(itr);
                    }
                    else {
                        itr++;
                    }
                }
            }
        }
    }
}

void Binder::registerServingNode(MacNodeId enbId, MacNodeId ueId)
{
    Enter_Method_Silent("registerServingNode");

    EV << "Binder : Registering UE " << ueId << " to serving eNB/gNB " << enbId << "\n";

    ASSERT(enbId == NODEID_NONE || getNodeTypeById(enbId) == ENODEB);
    ASSERT(getNodeTypeById(ueId) == UE);

    dMap_[enbId][ueId] = true;

    if (servingNode_.size() <= num(ueId))
        servingNode_.resize(num(ueId) + 1);
    servingNode_[num(ueId)] = enbId;
}

void Binder::unregisterServingNode(MacNodeId enbId, MacNodeId ueId)
{
    Enter_Method_Silent("unregisterServingNode");
    EV << "Binder : Unregistering UE " << ueId << " from serving eNodeB/gNodeB " << enbId << "\n";

    ASSERT(enbId == NODEID_NONE || getNodeTypeById(enbId) == ENODEB);
    ASSERT(getNodeTypeById(ueId) == UE);

    dMap_[enbId][ueId] = false;

    if (servingNode_.size() <= num(ueId))
        return;
    servingNode_[num(ueId)] = NODEID_NONE;
}

MacNodeId Binder::getServingNode(MacNodeId ueId)
{
    ASSERT(getNodeTypeById(ueId) == UE);
    return servingNode_[num(ueId)]; // overindexing extends vector with zeroes, which is fine
}

void Binder::registerMasterNode(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method_Silent("registerMasterNode");
    EV << "Binder : Registering slave " << slaveId << " to master " << masterId << "\n";

    ASSERT(masterId == NODEID_NONE || getNodeTypeById(masterId) == ENODEB);
    ASSERT(getNodeTypeById(slaveId) == ENODEB);
    ASSERT(masterId != slaveId);

    if (secondaryNodeToMasterNodeOrSelf_.size() <= num(slaveId))
        secondaryNodeToMasterNodeOrSelf_.resize(num(slaveId) + 1);
    secondaryNodeToMasterNodeOrSelf_[num(slaveId)] = (masterId != NODEID_NONE) ? masterId : slaveId;  // the "or self" bit
}

inline ostream& operator<<(ostream& os, const L3Address& addr) { return os << addr.str(); }
inline ostream& operator<<(ostream& os, const UeInfo& info) { return os << info.str(); }
inline ostream& operator<<(ostream& os, const EnbInfo& info) { return os << info.str(); }
inline ostream& operator<<(ostream& os, const BgTrafficManagerInfo& info) { return os << info.str(); }

void Binder::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        phyPisaData.setBlerShift(par("blerShift"));
        networkName_ = getSystemModule()->getName();

        // Add WATCH macros for all member variables
        WATCH(networkName_);
        WATCH_MAP(ipAddressToMacNodeId_);
        WATCH_MAP(ipAddressToNrMacNodeId_);
        // WATCH_MAP(nodeInfoMap_); // Commented out - contains complex NodeInfo structs that don't have stream operators
        WATCH_VECTOR(servingNode_);
        WATCH_VECTOR(secondaryNodeToMasterNodeOrSelf_);
        WATCH_SET(mecHostAddress_);
        WATCH_MAP(mecHostToUpfAddress_);
        // WATCH_MAP(extCellList_); // Commented out - contains vectors of ExtCell* pointers that don't have stream operators
        // WATCH_MAP(bgSchedulerList_); // Commented out - contains vectors of BackgroundScheduler* pointers that don't have stream operators
        WATCH_PTRVECTOR(enbList_); // Commented out - contains EnbInfo* pointers that don't have stream operators
        WATCH_PTRVECTOR(ueList_); // Commented out - contains UeInfo* pointers that don't have stream operators
        WATCH_PTRVECTOR(bgTrafficManagerList_); // Commented out - contains BgTrafficManagerInfo* pointers that don't have stream operators
        // WATCH_MAP(bgCellsInterferenceMatrix_); // Commented out - contains nested maps that don't have stream operators
        // WATCH_MAP(bgUesInterferenceMatrix_); // Commented out - contains nested maps that don't have stream operators
        WATCH(maxDataRatePerRb_);
        WATCH(macNodeIdCounterUe_);
        WATCH(macNodeIdCounterNrUe_);
        WATCH(macNodeIdCounterEnb_);
        // WATCH_MAP(dMap_); // Commented out - contains nested maps that don't have stream operators
        WATCH(totalBands_);
        // WATCH_MAP(componentCarriers_); // Commented out - contains complex CarrierInfo structs that don't have stream operators
        // WATCH_MAP(carrierUeMap_); // Commented out - contains sets that don't have stream operators
        WATCH_MAP(carrierFreqToNumerologyIndex_);
        WATCH_VECTOR(ueMaxNumerologyIndex_);
        // WATCH_MAP(ueNumerologyIndex_); // Commented out - contains sets that don't have stream operators
        // WATCH_MAP(ulTransmissionMap_); // Commented out - contains complex nested vectors that don't have stream operators
        WATCH(lastUpdateUplinkTransmissionInfo_);
        WATCH(lastUplinkTransmission_);
        // WATCH_MAP(x2ListeningPorts_); // Commented out - contains lists that don't have stream operators
        // WATCH_MAP(x2PeerAddress_); // Commented out - contains L3Address that doesn't have stream operator
        // WATCH_MAP(d2dPeeringMap_); // Commented out - contains nested maps that don't have stream operators
        // WATCH_MAP(multicastGroupMap_); // Commented out - contains sets that don't have stream operators
        WATCH_SET(multicastTransmitterSet_);
        WATCH_SET(ueHandoverTriggered_);
        // WATCH_MAP(handoverTriggered_); // Commented out - contains pairs that don't have stream operators
    }

    if (stage == inet::INITSTAGE_LAST) {
        maxDataRatePerRb_ = par("maxDataRatePerRb");

        // if avg interference enabled, compute CQIs
        computeAverageCqiForBackgroundUes();
    }
}

void Binder::finish()
{
    if (par("printTrafficGeneratorConfig").boolValue()) {
        // build filename
        std::stringstream outputFilenameStr;
        outputFilenameStr << "config";
        cConfigurationEx *configEx = getEnvir()->getConfigEx();

        const char *itervars = configEx->getVariable(CFGVAR_ITERATIONVARSF);
        outputFilenameStr << "-" << itervars << "repetition=" << configEx->getVariable("repetition") << ".ini";
        std::string outputFilename = outputFilenameStr.str();

        // open output file
        std::ofstream out(outputFilename);

        std::string toPrint;
        for (UeInfo *info : ueList_) {
            std::stringstream ss;

            if (info->id < NR_UE_MIN_ID)
                continue;

            MacNodeId cellId = info->cellId;
            int ueIndex = info->ue->getIndex();

            // get HARQ error rate
            LteMacBase *macUe = check_and_cast<LteMacBase *>(info->ue->getSubmodule("cellularNic")->getSubmodule("nrMac"));
            double harqErrorRateDl = macUe->getHarqErrorRate(DL);
            double harqErrorRateUl = macUe->getHarqErrorRate(UL);

            // get average CQI
            LtePhyUe *phyUe = check_and_cast<LtePhyUe *>(info->phy);
            double cqiDl = phyUe->getAverageCqi(DL);
            double cqiUl = phyUe->getAverageCqi(UL);
            double cqiVarianceDl = phyUe->getVarianceCqi(DL);
            double cqiVarianceUl = phyUe->getVarianceCqi(UL);

            if (info->cellId == MacNodeId(1)) {  //TODO magic number
                // the UE belongs to the central cell
                ss << "*.gnb.cellularNic.bgTrafficGenerator[0].bgUE[" << ueIndex << "].generator.rtxRateDl = " << harqErrorRateDl << "\n";
                ss << "*.gnb.cellularNic.bgTrafficGenerator[0].bgUE[" << ueIndex << "].generator.rtxRateUl = " << harqErrorRateUl << "\n";
                ss << "*.gnb.cellularNic.bgTrafficGenerator[0].bgUE[" << ueIndex << "].generator.cqiMeanDl = " << cqiDl << "\n";
                ss << "*.gnb.cellularNic.bgTrafficGenerator[0].bgUE[" << ueIndex << "].generator.cqiStddevDl = " << sqrt(cqiVarianceDl) << "\n";
                ss << "*.gnb.cellularNic.bgTrafficGenerator[0].bgUE[" << ueIndex << "].generator.cqiMeanUl = " << cqiUl << "\n";
                ss << "*.gnb.cellularNic.bgTrafficGenerator[0].bgUE[" << ueIndex << "].generator.cqiStddevUl = " << sqrt(cqiVarianceUl) << "\n";

                toPrint = ss.str();
            }
            else {
                MacNodeId bgCellId = cellId - 2;

                // the UE belongs to a background cell
                ss << "*.bgCell[" << bgCellId << "].bgTrafficGenerator.bgUE[" << ueIndex << "].generator.rtxRateDl = " << harqErrorRateDl << "\n";
                ss << "*.bgCell[" << bgCellId << "].bgTrafficGenerator.bgUE[" << ueIndex << "].generator.rtxRateUl = " << harqErrorRateUl << "\n";
                ss << "*.bgCell[" << bgCellId << "].bgTrafficGenerator.bgUE[" << ueIndex << "].generator.cqiMeanDl = " << cqiDl << "\n";
                ss << "*.bgCell[" << bgCellId << "].bgTrafficGenerator.bgUE[" << ueIndex << "].generator.cqiStddevDl = " << sqrt(cqiVarianceDl) << "\n";
                ss << "*.bgCell[" << bgCellId << "].bgTrafficGenerator.bgUE[" << ueIndex << "].generator.cqiMeanUl = " << cqiUl << "\n";
                ss << "*.bgCell[" << bgCellId << "].bgTrafficGenerator.bgUE[" << ueIndex << "].generator.cqiStddevUl = " << sqrt(cqiVarianceUl) << "\n";

                toPrint = ss.str();
            }

            out << toPrint;
        }

        out.close();
    }
}

cModule *Binder::getNodeModule(MacNodeId nodeId)
{
    auto it = nodeInfoMap_.find(nodeId);
    return it != nodeInfoMap_.end() ? it->second.moduleRef : nullptr;
}

LteMacBase *Binder::getMacFromMacNodeId(MacNodeId id)
{
    if (id == NODEID_NONE)
        return nullptr;

    auto it = nodeInfoMap_.find(id);
    if (it == nodeInfoMap_.end())
        return nullptr;

    // Check if MAC module is already cached
    if (it->second.macModule == nullptr) {
        // Cache the MAC module reference
        it->second.macModule = check_and_cast<LteMacBase *>(getMacByNodeId(id));
    }

    return it->second.macModule;
}

MacNodeId Binder::getNextHop(MacNodeId nodeId)
{
    return (nodeId == NODEID_NONE || getNodeTypeById(nodeId) == ENODEB) ? nodeId : getServingNode(nodeId);
}

MacNodeId Binder::getMasterNodeOrSelf(MacNodeId secondaryEnbId)
{
    ASSERT(secondaryEnbId == NODEID_NONE || getNodeTypeById(secondaryEnbId) == ENODEB);

    if (num(secondaryEnbId) >= secondaryNodeToMasterNodeOrSelf_.size())
        throw cRuntimeError("Binder::getMasterNode(): bad secondaryEnbId %hu", num(secondaryEnbId));
    return secondaryNodeToMasterNodeOrSelf_[num(secondaryEnbId)];
}

MacNodeId Binder::getSecondaryNode(MacNodeId masterEnbId)
{
    //TODO proper solution! (maintain reverse mapping)
    for (size_t i = 0; i < secondaryNodeToMasterNodeOrSelf_.size(); i++)
        if (secondaryNodeToMasterNodeOrSelf_[i] == masterEnbId && i != num(masterEnbId))  // exclude "self"!
            return MacNodeId(i);
    return NODEID_NONE;
}

void Binder::registerMecHost(const inet::L3Address& mecHostAddress)
{
    mecHostAddress_.insert(mecHostAddress);
}

void Binder::registerMecHostUpfAddress(const inet::L3Address& mecHostAddress, const inet::L3Address& gtpAddress)
{
    mecHostToUpfAddress_[mecHostAddress] = gtpAddress;
}

bool Binder::isMecHost(const inet::L3Address& mecHostAddress)
{
    return mecHostAddress_.find(mecHostAddress) != mecHostAddress_.end();
}

const inet::L3Address& Binder::getUpfFromMecHost(const inet::L3Address& mecHostAddress)
{
    if (mecHostToUpfAddress_.find(mecHostAddress) == mecHostToUpfAddress_.end())
        throw cRuntimeError("Binder::getUpfFromMecHost - address %s not found", mecHostAddress.str().c_str());
    return mecHostToUpfAddress_[mecHostAddress];
}

void Binder::registerModule(MacNodeId nodeId, cModule *module)
{
    auto it = nodeInfoMap_.find(nodeId);
    if (it != nodeInfoMap_.end()) {
        it->second.moduleRef = module;
    } else {
        // If node doesn't exist yet, create a new entry
        NodeInfo nodeInfo;
        nodeInfo.moduleRef = module;
        nodeInfoMap_[nodeId] = nodeInfo;
    }
}

cModule *Binder::getModuleByMacNodeId(MacNodeId nodeId)
{
    auto it = nodeInfoMap_.find(nodeId);
    if (it == nodeInfoMap_.end() || it->second.moduleRef == nullptr)
        throw cRuntimeError("Binder::getModuleByMacNodeId - node ID %d not found", num(nodeId));
    return it->second.moduleRef;
}

ConnectedUesMap Binder::getDeployedUes(MacNodeId localId, Direction dir)
{
    Enter_Method_Silent("getDeployedUes");
    return dMap_[localId];
}

simtime_t Binder::getLastUpdateUlTransmissionInfo()
{
    return lastUpdateUplinkTransmissionInfo_;
}

void Binder::initAndResetUlTransmissionInfo()
{
    if (lastUplinkTransmission_ < NOW - 2 * TTI) {
        // data structures have not been used in the last 2 time slots,
        // so they do not need to be updated.
        return;
    }

    for (auto& [timeSlot, transmissions] : ulTransmissionMap_) {
        // the second element (i.e., referring to the old time slot) becomes the first element
        if (!transmissions.empty())
            transmissions.erase(transmissions.begin());
    }
    lastUpdateUplinkTransmissionInfo_ = NOW;
}

void Binder::storeUlTransmissionMap(GHz carrierFreq, Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, LtePhyBase *phy, Direction dir)
{
    UeAllocationInfo info;
    info.nodeId = nodeId;
    info.cellId = cellId;
    info.phy = phy;
    info.dir = dir;
    info.trafficGen = nullptr;

    if (ulTransmissionMap_.find(carrierFreq) == ulTransmissionMap_.end() || ulTransmissionMap_[carrierFreq].size() == 0) {
        int numCarrierBands = componentCarriers_[carrierFreq].numBands;
        ulTransmissionMap_[carrierFreq].resize(2);
        ulTransmissionMap_[carrierFreq][PREV_TTI].resize(numCarrierBands);
        ulTransmissionMap_[carrierFreq][CURR_TTI].resize(numCarrierBands);
    }
    else if (ulTransmissionMap_[carrierFreq].size() == 1) {
        int numCarrierBands = componentCarriers_[carrierFreq].numBands;
        ulTransmissionMap_[carrierFreq].push_back(std::vector<std::vector<UeAllocationInfo>>());
        ulTransmissionMap_[carrierFreq][CURR_TTI].resize(numCarrierBands);
    }

    // for each allocated band, store the UE info
    for (const auto& [band, allocation] : rbMap[antenna]) {
        if (allocation > 0)
            ulTransmissionMap_[carrierFreq][CURR_TTI][band].push_back(info);
    }

    lastUplinkTransmission_ = NOW;
}

void Binder::storeUlTransmissionMap(GHz carrierFreq, Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, TrafficGeneratorBase *trafficGen, Direction dir)
{
    UeAllocationInfo info;
    info.nodeId = nodeId;
    info.cellId = cellId;
    info.phy = nullptr;
    info.dir = dir;
    info.trafficGen = trafficGen;

    if (ulTransmissionMap_.find(carrierFreq) == ulTransmissionMap_.end() || ulTransmissionMap_[carrierFreq].size() == 0) {
        int numCarrierBands = componentCarriers_[carrierFreq].numBands;
        ulTransmissionMap_[carrierFreq].resize(2);
        ulTransmissionMap_[carrierFreq][PREV_TTI].resize(numCarrierBands);
        ulTransmissionMap_[carrierFreq][CURR_TTI].resize(numCarrierBands);
    }
    else if (ulTransmissionMap_[carrierFreq].size() == 1) {
        int numCarrierBands = componentCarriers_[carrierFreq].numBands;
        ulTransmissionMap_[carrierFreq].push_back(std::vector<std::vector<UeAllocationInfo>>());
        ulTransmissionMap_[carrierFreq][CURR_TTI].resize(numCarrierBands);
    }

    // for each allocated band, store the UE info
    for (const auto& [band, allocation] : rbMap[antenna]) {
        if (allocation > 0)
            ulTransmissionMap_[carrierFreq][CURR_TTI][band].push_back(info);
    }

    lastUplinkTransmission_ = NOW;
}

const std::vector<std::vector<UeAllocationInfo>> *Binder::getUlTransmissionMap(GHz carrierFreq, UlTransmissionMapTTI t)
{
    if (ulTransmissionMap_.find(carrierFreq) == ulTransmissionMap_.end() || t >= ulTransmissionMap_[carrierFreq].size()) {
        return nullptr;
    }

    return &(ulTransmissionMap_[carrierFreq].at(t));
}

void Binder::registerX2Port(X2NodeId nodeId, int port)
{
    if (x2ListeningPorts_.find(nodeId) == x2ListeningPorts_.end()) {
        // no port has yet been registered
        std::list<int> ports;
        ports.push_back(port);
        x2ListeningPorts_[nodeId] = ports;
    }
    else {
        x2ListeningPorts_[nodeId].push_back(port);
    }
}

int Binder::getX2Port(X2NodeId nodeId)
{
    if (x2ListeningPorts_.find(nodeId) == x2ListeningPorts_.end())
        throw cRuntimeError("Binder::getX2Port - No ports available on node %hu", num(nodeId));

    int port = x2ListeningPorts_[nodeId].front();
    x2ListeningPorts_[nodeId].pop_front();
    return port;
}

Cqi Binder::meanCqi(std::vector<Cqi> bandCqi, MacNodeId id, Direction dir)
{
    Cqi mean = 0;
    for (Cqi value : bandCqi) {
        mean += value;
    }
    mean /= bandCqi.size();

    if (mean == 0)
        mean = 1;

    return mean;
}

Cqi Binder::medianCqi(std::vector<Cqi> bandCqi, MacNodeId id, Direction dir)
{
    std::sort(bandCqi.begin(), bandCqi.end());

    int medianPoint = bandCqi.size() / 2;

    EV << "Binder::medianCqi - median point is " << bandCqi.size() << "/2 = " << medianPoint << ". MedianCqi = " << bandCqi[medianPoint] << endl;

    return bandCqi[medianPoint];
}

bool Binder::isValidNodeId(MacNodeId  nodeId) const
{
    if (nodeId >= UE_MIN_ID && nodeId < MacNodeId(macNodeIdCounterUe_))
        return true;
    if (nodeId >= NR_UE_MIN_ID && nodeId < MacNodeId(macNodeIdCounterNrUe_))
        return true;
    if (nodeId >= ENB_MIN_ID && nodeId < MacNodeId(macNodeIdCounterEnb_))
        return true;
    return false;
}

LteD2DMode Binder::computeD2DCapability(MacNodeId src, MacNodeId dst)
{
    LteMacBase *dstMac = getMacFromMacNodeId(dst);
    if (dstMac->isD2DCapable()) {
        // set the initial mode
        if (servingNode_[num(src)] == servingNode_[num(dst)]) {
            // if served by the same cell, then the mode is selected according to the corresponding parameter
            LteMacBase *srcMac = getMacFromMacNodeId(src);
            inet::NetworkInterface *srcNic = getContainingNicModule(srcMac);
            bool d2dInitialMode = srcNic->hasPar("d2dInitialMode") ? srcNic->par("d2dInitialMode").boolValue() : false;
            return d2dInitialMode ? DM : IM;
        }
        else {
            // if served by different cells, then the mode can be IM only
            return IM;
        }
    }
    else {
        // this is not a D2D-capable flow
        return NONE;
    }
}

bool Binder::checkD2DCapability(MacNodeId src, MacNodeId dst)
{
    ASSERT(getNodeTypeById(src) == UE && isValidNodeId(src));
    ASSERT(getNodeTypeById(dst) == UE && isValidNodeId(dst));

    // if the entry is missing, check if the receiver is D2D capable and update the map
    if (!containsKey(d2dPeeringMap_, src) || !containsKey(d2dPeeringMap_[src], dst))
    {
        LteD2DMode mode = computeD2DCapability(src, dst);
        if (mode == NONE) {
            EV << "Binder::checkD2DCapability - UE " << src << " may not transmit to UE " << dst << " using D2D (UE " << dst << " is not D2D capable)" << endl;
        }
        else {
            EV << "Binder::checkD2DCapability - UE " << src << " may transmit to UE " << dst << " using D2D (current mode " << (mode == DM ? "DM)" : "IM)") << endl;
        }
        d2dPeeringMap_[src][dst] = mode;
        return mode != NONE;
    }

    // if an entry is present, and it is not NONE, this is a D2D-capable flow
    return d2dPeeringMap_[src][dst] != NONE;
}

bool Binder::getD2DCapability(MacNodeId src, MacNodeId dst)
{
    ASSERT(getNodeTypeById(src) == UE && isValidNodeId(src));
    ASSERT(getNodeTypeById(dst) == UE && isValidNodeId(dst));

    // return true if the entry exists and it is not NONE, no matter if it is DM or IM
    return containsKey(d2dPeeringMap_, src) && containsKey(d2dPeeringMap_[src], dst) && d2dPeeringMap_[src][dst] != NONE;
}

std::map<MacNodeId, std::map<MacNodeId, LteD2DMode>> *Binder::getD2DPeeringMap()
{
    return &d2dPeeringMap_;
}

LteD2DMode Binder::getD2DMode(MacNodeId src, MacNodeId dst)
{
    if (!getD2DCapability(src,dst))
        throw cRuntimeError("Binder::getD2DMode - Node Id not valid. Src %hu Dst %hu", num(src), num(dst));

    return d2dPeeringMap_[src][dst];
}

bool Binder::isFrequencyReuseEnabled(MacNodeId nodeId)
{
    // a d2d-enabled UE can use frequency reuse if it can communicate using DM with all its peers
    // in fact, the scheduler does not know to which UE it will communicate when it grants some RBs
    if (!containsKey(d2dPeeringMap_, nodeId) || d2dPeeringMap_[nodeId].empty())
        return false;

    for (const auto& [_, mode] : d2dPeeringMap_[nodeId])
        if (mode == IM)
            return false;
    return true;
}

void Binder::joinMulticastGroup(MacNodeId nodeId, int32_t groupId)
{
    nodeGroupMemberships_[nodeId].insert(groupId);
}

bool Binder::isInMulticastGroup(MacNodeId nodeId, int32_t groupId)
{
    return inet::containsKey(nodeGroupMemberships_, nodeId) && inet::contains(nodeGroupMemberships_[nodeId], groupId);
}

void Binder::addD2DMulticastTransmitter(MacNodeId nodeId)
{
    multicastTransmitterSet_.insert(nodeId);
}

std::set<MacNodeId>& Binder::getD2DMulticastTransmitters()
{
    return multicastTransmitterSet_;
}

void Binder::updateUeInfoCellId(MacNodeId id, MacCellId newCellId)
{
    for (UeInfo *ue : ueList_) {
        if (ue->id == id) {
            ue->cellId = newCellId;
            return;
        }
    }
}

void Binder::addUeHandoverTriggered(MacNodeId nodeId)
{
    ueHandoverTriggered_.insert(nodeId);
}

bool Binder::hasUeHandoverTriggered(MacNodeId nodeId)
{
    return ueHandoverTriggered_.find(nodeId) != ueHandoverTriggered_.end();
}

void Binder::removeUeHandoverTriggered(MacNodeId nodeId)
{
    ueHandoverTriggered_.erase(nodeId);
}

void Binder::addHandoverTriggered(MacNodeId nodeId, MacNodeId srcId, MacNodeId destId)
{
    handoverTriggered_[nodeId] = {srcId, destId};
}

const std::pair<MacNodeId, MacNodeId> *Binder::getHandoverTriggered(MacNodeId nodeId)
{
    if (handoverTriggered_.find(nodeId) == handoverTriggered_.end())
        return nullptr;
    return &handoverTriggered_[nodeId];
}

void Binder::removeHandoverTriggered(MacNodeId nodeId)
{
    auto it = handoverTriggered_.find(nodeId);
    if (it != handoverTriggered_.end())
        handoverTriggered_.erase(it);
}

void Binder::computeAverageCqiForBackgroundUes()
{
    EV << " ===== Binder::computeAverageCqiForBackgroundUes - START =====" << endl;

    // initialize interference matrix
    for (unsigned int i = 0; i < bgTrafficManagerList_.size(); i++) {
        std::map<unsigned int, double> tmp;
        for (unsigned int j = 0; j < bgTrafficManagerList_.size(); j++)
            tmp[j] = 0.0;

        bgCellsInterferenceMatrix_[i] = tmp;
    }

    // update interference until "condition" becomes true
    // condition = at least one value of interference is above the threshold and
    //             we did not reach the maximum number of iteration
    bool condition = true;
    const int MAX_INTERFERENCE_CHECK = 10;

    // interference check needs to be done at least 2 times (1 setup)
    unsigned int countInterferenceCheck = 0;

    /*
     * Compute SINR for each user and interference between cells (for DL) and UEs (for UL)
     * This will be done in steps:
     * 1) compute SINR without interference and update block usage
     * 2) while average interference variation between 2 consecutive steps is above a certain threshold
     *  - Update interference
     *  - Compute SINR with interference
     *  - Update cell block usage according to connected UEs
     */
    while (condition) {
        countInterferenceCheck++;
        EV << " * ITERATION " << countInterferenceCheck << " *" << endl;

        // --- MAIN Interference Check Cycle --- //

        // loop through the BackgroundTrafficManagers (one per cell)
        for (unsigned int bgTrafficManagerId = 0; bgTrafficManagerId < bgTrafficManagerList_.size(); bgTrafficManagerId++) {
            BgTrafficManagerInfo *info = bgTrafficManagerList_.at(bgTrafficManagerId);
            if (!(info->init))
                continue;

            IBackgroundTrafficManager *bgTrafficManager = info->bgTrafficManager;
            unsigned int numBands = bgTrafficManager->getNumBands();

            //---------------------------------------------------------------------
            // STEP 1: update mutual interference
            // for iterations after the first one, update the interference before analyzing a whole cell
            // Note that it makes no sense to compute this at the first iteration when the cell is allocating
            // zero blocks still
            if (countInterferenceCheck > 1) {
                updateMutualInterference(bgTrafficManagerId, numBands, DL);
                updateMutualInterference(bgTrafficManagerId, numBands, UL);
            }

            double cellRbsDl = 0;
            double cellRbsUl = 0;

            // Compute the SINR for each UE within the cell
            auto bgUes_it = bgTrafficManager->getBgUesBegin();
            auto bgUes_et = bgTrafficManager->getBgUesEnd();
            while (bgUes_it != bgUes_et) {
                TrafficGeneratorBase *bgUe = *bgUes_it;

                //---------------------------------------------------------------------
                // STEP 2: compute SINR
                bool losStatus = false;
                inet::Coord bsCoord = bgTrafficManager->getBsCoord();
                inet::Coord bgUeCoord = bgUe->getCoord();
                int bgUeId = bgUe->getId();
                double bsTxPower = bgTrafficManager->getBsTxPower();
                double bgUeTxPower = bgUe->getTxPwr();

                double sinrDl = computeSinr(bgTrafficManagerId, bgUeId, bsTxPower, bsCoord, bgUeCoord, DL, losStatus);
                double sinrUl = computeSinr(bgTrafficManagerId, bgUeId, bgUeTxPower, bgUeCoord, bsCoord, UL, losStatus);

                // update CQI for the bg UE
                bgUe->setCqiFromSinr(sinrDl, DL);
                bgUe->setCqiFromSinr(sinrUl, UL);

                //---------------------------------------------------------------------
                // STEP 3: update block allocation

                // obtain UE load request (in bits/second)
                double ueLoadDl = bgUe->getAvgLoad(DL) * 8;
                double ueLoadUl = bgUe->getAvgLoad(UL) * 8;

                // convert the UE load request to RBs based on SINR
                double ueRbsDl = computeRequestedRbsFromSinr(sinrDl, ueLoadDl);
                double ueRbsUl = computeRequestedRbsFromSinr(sinrUl, ueLoadUl);

                if (ueRbsDl < 0 || ueRbsUl < 0) {
                    EV << "Error! Computed negative requested RBs DL[" << ueRbsDl << "] UL[" << ueRbsUl << "]" << endl;
                    throw cRuntimeError("Binder::computeAverageCqiForBackgroundUes - Error! Computed negative requested RBs\n");
                }

                // check if there is room for ueRbs
                ueRbsDl = (ueRbsDl > numBands) ? numBands : ueRbsDl;
                ueRbsUl = (ueRbsUl > numBands) ? numBands : ueRbsUl;

                // update cell load
                cellRbsDl += ueRbsDl;
                cellRbsUl += ueRbsUl;

                // update allocation for the UL
                info->allocatedRbsUeUl.at(bgUeId) = ueRbsUl;

                // update allocation for the DL only at the first iteration
                if (countInterferenceCheck == 1) {
                    info->allocatedRbsDl = (cellRbsDl > numBands) ? numBands : cellRbsDl;
                    info->allocatedRbsUl = (cellRbsUl > numBands) ? numBands : cellRbsUl;
                }

                ++bgUes_it;
            }

            // update allocation element for this background traffic manager
            info->allocatedRbsDl = (cellRbsDl > numBands) ? numBands : cellRbsDl;
            info->allocatedRbsUl = (cellRbsUl > numBands) ? numBands : cellRbsUl;

            // if the total cellRbsUl is higher than numBands, then scale the allocation for all UEs
            if (cellRbsUl > numBands) {
                double scaleFactor = (double)numBands / cellRbsUl;
                for (double & i : info->allocatedRbsUeUl)
                    i *= scaleFactor;
            }
        }

        EV << "* END ITERATION " << countInterferenceCheck << endl;
        if (countInterferenceCheck > MAX_INTERFERENCE_CHECK)
            condition = false;
    }

    EV << " ===== Binder::computeAverageCqiForBackgroundUes - END =====" << endl;
}

void Binder::updateMutualInterference(unsigned int bgTrafficManagerId, unsigned int numBands, Direction dir)
{
    EV << "Binder::updateMutualInterference - computing interference for traffic manager " << bgTrafficManagerId << " dir[" << dirToA(dir) << "]" << endl;

    double ownRbs, extRbs; // current RBs allocation
    BgTrafficManagerInfo *ownInfo = bgTrafficManagerList_.at(bgTrafficManagerId);

    if (dir == DL) {
        // obtain current allocation info
        ownRbs = ownInfo->allocatedRbsDl;

        // loop through the BackgroundTrafficManagers (one per cell)
        for (unsigned int extId = 0; extId < bgTrafficManagerList_.size(); extId++) {
            // skip own interference
            if (extId == bgTrafficManagerId)
                continue;

            // obtain current allocation info for interfering bg cell
            BgTrafficManagerInfo *extInfo = bgTrafficManagerList_.at(extId);
            extRbs = extInfo->allocatedRbsDl;

            double newOverlapPercentage = computeInterferencePercentageDl(ownRbs, extRbs, numBands);

            EV << "ownId[" << bgTrafficManagerId << "] extId[" << extId << "] - ownRbs[" << ownRbs << "] extRbs[" << extRbs << "] - overlap[" << newOverlapPercentage << "]" << endl;

            // update interference
            bgCellsInterferenceMatrix_[bgTrafficManagerId][extId] = newOverlapPercentage;
        } // end ext-cell computation
    }
    else {
        // for each UE in the BG cell
        auto bgUes_it = ownInfo->bgTrafficManager->getBgUesBegin();
        auto bgUes_et = ownInfo->bgTrafficManager->getBgUesEnd();
        while (bgUes_it != bgUes_et) {
            int bgUeId = (*bgUes_it)->getId();
            uint32_t globalBgUeId = ((uint32_t)bgTrafficManagerId << 16) | (uint32_t)bgUeId;

            // RBs allocated to the BG UE
            ownRbs = ownInfo->allocatedRbsUeUl.at(bgUeId);

            // loop through the BackgroundTrafficManagers (one per cell)
            for (unsigned int extId = 0; extId < bgTrafficManagerList_.size(); extId++) {
                // skip own interference
                if (extId == bgTrafficManagerId)
                    continue;

                // obtain current allocation info for interfering bg cell
                BgTrafficManagerInfo *extInfo = bgTrafficManagerList_.at(extId);

                // for each UE in the ext BG cell
                auto extBgUes_it = extInfo->bgTrafficManager->getBgUesBegin();
                auto extBgUes_et = extInfo->bgTrafficManager->getBgUesEnd();
                while (extBgUes_it != extBgUes_et) {
                    int extBgUeId = (*extBgUes_it)->getId();
                    uint32_t globalExtBgUeId = ((uint32_t)extId << 16) | (uint32_t)extBgUeId;

                    // RBs allocated to the interfering BG UE
                    extRbs = extInfo->allocatedRbsUeUl.at(extBgUeId);

                    double newOverlapPercentage = computeInterferencePercentageUl(ownRbs, extRbs, ownInfo->allocatedRbsUl, extInfo->allocatedRbsUl);

                    EV << "ownId[" << globalBgUeId << "] extId[" << globalExtBgUeId << "] - ownRbs[" << ownRbs << "] extRbs[" << extRbs << "] - overlap[" << newOverlapPercentage << "]" << endl;

                    // update interference
                    if (bgUesInterferenceMatrix_.find(globalBgUeId) == bgUesInterferenceMatrix_.end()) {
                        std::map<unsigned int, double> newMap;
                        bgUesInterferenceMatrix_[globalBgUeId] = newMap;
                    }
                    else
                        bgUesInterferenceMatrix_[globalBgUeId][globalExtBgUeId] = newOverlapPercentage;

                    ++extBgUes_it;
                }
            } // end ext-cell computation

            ++bgUes_it;
        }
    }
}

double Binder::computeInterferencePercentageDl(double n, double k, unsigned int numBands)
{
    // assuming that the BS allocates starting from the first RB in DL
    if (n == 0)
        return 0;

    double min = (n < k) ? n : k;
    return (double)min / n;
}

double Binder::computeInterferencePercentageUl(double n, double k, double nTotal, double kTotal)
{
    // assuming the allocation of one UE is random (within the total allocated by its cell, not the total available bands)

    double max = (nTotal > kTotal) ? nTotal : kTotal;
    if (max == 0)
        return 0.0;

    return (double)k / max;
}

double Binder::computeSinr(unsigned int bgTrafficManagerId, int bgUeId, double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus)
{
    double sinr;

    BgTrafficManagerInfo *info = bgTrafficManagerList_.at(bgTrafficManagerId);
    IBackgroundTrafficManager *bgTrafficManager = info->bgTrafficManager;

    // TODO configure forceCellLos from config files
    bool ownCellLos = losStatus;
    bool otherLos = losStatus;

    // compute good signal received power
    double recvSignalDbm = bgTrafficManager->getReceivedPower_bgUe(txPower, txPos, rxPos, dir, ownCellLos);

    double totInterference = 0.0;

    // loop through the BackgroundTrafficManagers (one per cell)
    for (unsigned int extId = 0; extId < bgTrafficManagerList_.size(); extId++) {
        // skip own interference
        if (extId == bgTrafficManagerId)
            continue;

        BgTrafficManagerInfo *extInfo = bgTrafficManagerList_.at(extId);

        inet::Coord interfCoord;
        double interfTxPower;
        double overlapPercentage;
        double interferenceDbm = 0.0;
        if (dir == DL) {
            interfCoord = (extInfo->bgTrafficManager)->getBsCoord();
            interfTxPower = (extInfo->bgTrafficManager)->getBsTxPower();

            overlapPercentage = bgCellsInterferenceMatrix_[bgTrafficManagerId][extId];
            if (overlapPercentage > 0)
                interferenceDbm = (extInfo->bgTrafficManager)->getReceivedPower_bgUe(interfTxPower, interfCoord, rxPos, dir, otherLos);

            // TODO? save the interference into a matrix, so that it can be used to evaluate if convergence has been reached

            totInterference += (dBmToLinear(interferenceDbm) * overlapPercentage);
        }
        else {
            uint32_t globalBgUeId = ((uint32_t)bgTrafficManagerId << 16) | (uint32_t)bgUeId;

            // for each UE in the ext BG cell
            auto extBgUes_it = extInfo->bgTrafficManager->getBgUesBegin();
            auto extBgUes_et = extInfo->bgTrafficManager->getBgUesEnd();
            while (extBgUes_it != extBgUes_et) {
                int extBgUeId = (*extBgUes_it)->getId();
                uint32_t globalExtBgUeId = ((uint32_t)extId << 16) | (uint32_t)extBgUeId;

                interfCoord = (*extBgUes_it)->getCoord();
                interfTxPower = (*extBgUes_it)->getTxPwr();

                overlapPercentage = bgUesInterferenceMatrix_[globalBgUeId][globalExtBgUeId];
                if (overlapPercentage > 0)
                    interferenceDbm = (extInfo->bgTrafficManager)->getReceivedPower_bgUe(interfTxPower, interfCoord, rxPos, dir, otherLos);

                totInterference += (dBmToLinear(interferenceDbm) * overlapPercentage);

                // TODO? save the interference into a matrix, so that it can be used to evaluate if convergence has been reached

                ++extBgUes_it;
            }
        }
    }

    double thermalNoise = -104.5;  // dBm TODO take it from the channel model
    double linearNoise = dBmToLinear(thermalNoise);
    double denomDbm = linearToDBm(linearNoise + totInterference);
    sinr = recvSignalDbm - denomDbm;

    return sinr;
}

double Binder::computeRequestedRbsFromSinr(double sinr, double reqLoad)
{
    // TODO choose appropriate values for these constants
    const double MIN_SINR = -5.5;
    const double MAX_SINR = 25.5;

    // we let a UE have a minimum CQI (2) even when SINR is too low (this is what our scheduler does)
    if (sinr <= -3.5)
        sinr = -3.5;
    if (sinr > MAX_SINR)
        sinr = MAX_SINR;

    double sinrRange = MAX_SINR - MIN_SINR;

    double normalizedSinr = (sinr + fabs(MIN_SINR)) / sinrRange;

    double bitRate = normalizedSinr * maxDataRatePerRb_;

    double rbs = reqLoad / bitRate;

    EV << "Binder::computeRequestedRbsFromSinr - sinr[" << sinr << "] - bitRate[" << bitRate << "] - rbs[" << rbs << "]" << endl;

    return rbs;
}

/*
 * @author Alessandro Noferi
 */

void Binder::addUeCollectorToEnodeB(MacNodeId ue, UeStatsCollector *ueCollector, MacNodeId cell)
{
    EV << "LteBinder::addUeCollector" << endl;
    cModule *enb = nullptr;
    BaseStationStatsCollector *enbColl = nullptr;

    // Check if the collector is already present in a cell
    for (auto & enbInfo : enbList_) {
        enb = enbInfo->eNodeB;
        if (enb->getSubmodule("collector") != nullptr) {
            enbColl = check_and_cast<BaseStationStatsCollector *>(enb->getSubmodule("collector"));
            if (enbColl->hasUeCollector(ue)) {
                EV << "LteBinder::addUeCollector - UeCollector for node [" << ue << "] already present in eNodeB [" << enbInfo->id << "]" << endl;
                throw cRuntimeError("LteBinder::addUeCollector - UeCollector for node [%hu] already present in eNodeB [%hu]", num(ue), num(enbInfo->id));
            }
        }
        else {
            EV << "LteBinder::addUeCollector - eNodeB [" << enbInfo->id << "] does not have the eNodeBStatsCollector" << endl;
        }
    }

    // No cell has the UeCollector, add it
    enb = getModuleByMacNodeId(cell);
    if (enb->getSubmodule("collector") != nullptr) {
        enbColl = check_and_cast<BaseStationStatsCollector *>(enb->getSubmodule("collector"));
        enbColl->addUeCollector(ue, ueCollector);
        EV << "LteBinder::addUeCollector - UeCollector for node [" << ue << "] added to eNodeB [" << cell << "]" << endl;
    }
    else {
        EV << "LteBinder::addUeCollector - eNodeB [" << cell << "] does not have the eNodeBStatsCollector." <<
            " UeCollector for node [" << ue << "] NOT added to eNodeB [" << cell << "]" << endl;
    }
}

void Binder::moveUeCollector(MacNodeId ue, MacCellId oldCell, MacCellId newCell)
{
    EV << "LteBinder::moveUeCollector" << endl;
    RanNodeType oldCellType = getBaseStationTypeById(oldCell);
    RanNodeType newCellType = getBaseStationTypeById(newCell);

    // Get and remove the UeCollector from the old cell
    cModule *oldEnb = getModuleByMacNodeId(oldCell); // eNodeB module
    BaseStationStatsCollector *enbColl = nullptr;
    UeStatsCollector *ueColl = nullptr;
    if (oldEnb->getSubmodule("collector") != nullptr) {
        enbColl = check_and_cast<BaseStationStatsCollector *>(oldEnb->getSubmodule("collector"));
        if (enbColl->hasUeCollector(ue)) {
            ueColl = enbColl->getUeCollector(ue);
            ueColl->resetStats();
            enbColl->removeUeCollector(ue);
        }
        else {
            throw cRuntimeError("LteBinder::moveUeCollector - UeStatsCollector of node [%hu] not present in eNodeB [%hu]", num(ue), num(oldCell));
        }
    }
    else {
        throw cRuntimeError("LteBinder::moveUeCollector - eNodeBStatsCollector not present in eNodeB [%hu]", num(oldCell));
    }
    // If the two base stations are the same type, just move the collector
    if (oldCellType == newCellType) {
        addUeCollectorToEnodeB(ue, ueColl, newCell);
    }
    else {
        if (newCellType == GNODEB) {
            // Retrieve NrUeCollector
            cModule *ueModule = getModuleByMacNodeId(ue);
            if (ueModule->findSubmodule("nrUeCollector") == -1)
                ueColl = check_and_cast<UeStatsCollector *>(ueModule->getSubmodule("nrUeCollector"));
            else
                throw cRuntimeError("LteBinder::moveUeCollector - Ue [%hu] does not have an 'nrUeCollector' submodule required for the gNB", num(ue));
            addUeCollectorToEnodeB(ue, ueColl, newCell);
        }
        else if (newCellType == ENODEB) {
            // Retrieve NrUeCollector
            cModule *ueModule = getModuleByMacNodeId(ue);
            if (ueModule->findSubmodule("ueCollector") == -1)
                ueColl = check_and_cast<UeStatsCollector *>(ueModule->getSubmodule("ueCollector"));
            else
                throw cRuntimeError("LteBinder::moveUeCollector - Ue [%hu] does not have an 'ueCollector' submodule required for the eNB", num(ue));
            addUeCollectorToEnodeB(ue, ueColl, newCell);
        }
        else {
            throw cRuntimeError("LteBinder::moveUeCollector - The new cell is not a cell [%u]", newCellType);
        }
    }
}

RanNodeType Binder::getBaseStationTypeById(MacNodeId cellId)
{
    cModule *module = getModuleByMacNodeId(cellId);
    std::string nodeType;
    if (module->hasPar("nodeType")) {
        nodeType = module->par("nodeType").stdstringValue();
        return static_cast<RanNodeType>(cEnum::get("simu5g::RanNodeType")->lookup(nodeType.c_str()));
    }
    else {
        return UNKNOWN_NODE_TYPE;
    }
}

CellInfo *Binder::getCellInfoByNodeId(MacNodeId nodeId)
{
    // Check if it is an eNodeB
    // function GetNextHop returns nodeId
    MacNodeId id = getNextHop(nodeId);
    cModule *module = getNodeModule(id);
    return module ? check_and_cast<CellInfo *>(module->getSubmodule("cellInfo")) : nullptr;
}

cModule *Binder::getPhyByNodeId(MacNodeId nodeId)
{
    // UE might have left the simulation, return NULL in this case
    // since we do not have a MAC-Module anymore
    cModule *module = getNodeModule(nodeId);
    if (module == nullptr) {
        return nullptr;
    }
    if (isNrUe(nodeId))
        return module->getSubmodule("cellularNic")->getSubmodule("nrPhy");
    return module->getSubmodule("cellularNic")->getSubmodule("phy");
}

cModule *Binder::getMacByNodeId(MacNodeId nodeId)
{
    // UE might have left the simulation, return NULL in this case
    // since we do not have a MAC-Module anymore
    cModule *module = getNodeModule(nodeId);
    if (module == nullptr) {
        return nullptr;
    }
    if (isNrUe(nodeId))
        return module->getSubmodule("cellularNic")->getSubmodule("nrMac");
    return module->getSubmodule("cellularNic")->getSubmodule("mac");
}

cModule *Binder::getRlcByNodeId(MacNodeId nodeId, LteRlcType rlcType)
{
    cModule *module = getNodeModule(nodeId);
    if (module == nullptr) {
        return nullptr;
    }
    if (isNrUe(nodeId))
        return module->getSubmodule("cellularNic")->getSubmodule("nrRlc")->getSubmodule(rlcTypeToA(rlcType).c_str());
    return module->getSubmodule("cellularNic")->getSubmodule("rlc")->getSubmodule(rlcTypeToA(rlcType).c_str());
}

cModule *Binder::getPdcpByNodeId(MacNodeId nodeId)
{
    // UE might have left the simulation, return NULL in this case
    // since we do not have a MAC-Module anymore
    cModule *module = getNodeModule(nodeId);
    if (module == nullptr) {
        return nullptr;
    }
    return module->getSubmodule("cellularNic")->getSubmodule("pdcp");
}

} //namespace
