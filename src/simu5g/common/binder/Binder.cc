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
#include "simu5g/stack/rrc/Rrc.h"

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
        carrierUeMap_[carrierFrequency] = {};
    }
}

void Binder::registerCarrierUe(GHz carrierFrequency, unsigned int numerologyIndex, MacNodeId ueId)
{
    // check if carrier exists in the system
    CarrierUeMap::iterator it = carrierUeMap_.find(carrierFrequency);
    if (it == carrierUeMap_.end())
        throw cRuntimeError("Binder::registerCarrierUe - Carrier [%gGHz] not found (missing registerCarrier call?)", carrierFrequency.get());

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

void Binder::registerNode(MacNodeId nodeId, cModule *nodeModule, RanNodeType type, bool isNr)
{
    Enter_Method_Silent();

    // validate input
    if (nodeInfoMap_.find(nodeId) != nodeInfoMap_.end())
        throw cRuntimeError("Cannot register node %s in Binder: macNodeId %d already occupied", nodeModule->getFullPath().c_str(), nodeId);

    if (type == NODEB) {
        if (getNodeTypeById(nodeId) != NODEB)
            throw cRuntimeError("Cannot register node %s in Binder: Wrong macNodeId %d: Does not correspond to the range Simu5G reserves for eNodeB/gNodeB nodes", nodeModule->getFullPath().c_str(), num(nodeId));
    }
    else if (type == UE) {
        if (getNodeTypeById(nodeId) != UE)
            throw cRuntimeError("Cannot register node %s in Binder: Wrong macNodeId %d: Does not correspond to the range Simu5G reserves for UE nodes", nodeModule->getFullPath().c_str(), num(nodeId));
        if (isNr != (num(nodeId) >= NR_UE_MIN_ID))
            throw cRuntimeError("Cannot register node %s in Binder: Wrong macNodeId %d: Technology (LTE/NR) mismatch", nodeModule->getFullPath().c_str(), num(nodeId));
    }
    else {
        throw cRuntimeError("Cannot register node %s in Binder: Wrong node type: Expected UE or NODEB, but got %d", nodeModule->getFullPath().c_str(), type);
    }

    EV << "Binder : Registering module " << nodeModule->getFullPath() << " with MacNodeId " << nodeId << "\n";

    // registering new node
    NodeInfo nodeInfo;
    nodeInfo.moduleRef = nodeModule;
    nodeInfoMap_[nodeId] = nodeInfo;
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

    ASSERT(enbId == NODEID_NONE || getNodeTypeById(enbId) == NODEB);
    ASSERT(getNodeTypeById(ueId) == UE);

    if (servingNode_.size() <= num(ueId))
        servingNode_.resize(num(ueId) + 1);
    servingNode_[num(ueId)] = enbId;
}

void Binder::unregisterServingNode(MacNodeId enbId, MacNodeId ueId)
{
    Enter_Method_Silent("unregisterServingNode");
    EV << "Binder : Unregistering UE " << ueId << " from serving eNodeB/gNodeB " << enbId << "\n";

    ASSERT(enbId == NODEID_NONE || getNodeTypeById(enbId) == NODEB);
    ASSERT(getNodeTypeById(ueId) == UE);

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

    ASSERT(masterId == NODEID_NONE || getNodeTypeById(masterId) == NODEB);
    ASSERT(getNodeTypeById(slaveId) == NODEB);
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

            if (num(info->id) < NR_UE_MIN_ID)
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
                MacNodeId bgCellId = MacNodeId(num(cellId) - 2);

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
    return (nodeId == NODEID_NONE || getNodeTypeById(nodeId) == NODEB) ? nodeId : getServingNode(nodeId);
}

MacNodeId Binder::getMasterNodeOrSelf(MacNodeId secondaryEnbId)
{
    ASSERT(secondaryEnbId == NODEID_NONE || getNodeTypeById(secondaryEnbId) == NODEB);

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

MacNodeId Binder::getUeNodeId(MacNodeId ue, bool isNr)
{
    //TODO better impl! mapping via IP address is not ideal.
    if (!isValidNodeId(ue) || getNodeTypeById(ue) != UE)
        throw cRuntimeError("Binder::getUeNodeId(): bad ueId %hu", num(ue));

    inet::Ipv4Address ueIpAddr = getIPv4Address(ue);
    if (ueIpAddr == inet::Ipv4Address::UNSPECIFIED_ADDRESS)
        throw cRuntimeError("Binder::getUeNodeId(): no IP address for UE %hu", num(ue));

    if (isNr) {
        // Request for NR nodeId
        if (ipAddressToNrMacNodeId_.find(ueIpAddr) != ipAddressToNrMacNodeId_.end())
            return ipAddressToNrMacNodeId_[ueIpAddr];
    }
    else {
        // Request for LTE nodeId
        if (ipAddressToMacNodeId_.find(ueIpAddr) != ipAddressToMacNodeId_.end())
            return ipAddressToMacNodeId_[ueIpAddr];
    }

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

cModule *Binder::getModuleByMacNodeId(MacNodeId nodeId)
{
    auto it = nodeInfoMap_.find(nodeId);
    if (it == nodeInfoMap_.end() || it->second.moduleRef == nullptr)
        throw cRuntimeError("Binder::getModuleByMacNodeId - node ID %d not found", num(nodeId));
    return it->second.moduleRef;
}

std::vector<MacNodeId> Binder::getDeployedUes(MacNodeId enbNodeId)
{
    ASSERT(getNodeTypeById(enbNodeId) == NODEB);

    std::vector<MacNodeId> connectedUes;
    for (auto& [nodeId, nodeInfo] : nodeInfoMap_)
        if (nodeInfo.moduleRef != nullptr && getNodeTypeById(nodeId) == UE && servingNode_.size() > num(nodeId) && servingNode_[num(nodeId)] == enbNodeId)
            connectedUes.push_back(nodeId);

    return connectedUes;
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
    return nodeInfoMap_.find(nodeId) != nodeInfoMap_.end();
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

void Binder::joinMulticastGroup(MacNodeId nodeId, MacNodeId multicastDestId)
{
    nodeGroupMemberships_[nodeId].insert(multicastDestId);
}

bool Binder::isInMulticastGroup(MacNodeId nodeId, MacNodeId multicastDestId)
{
    return inet::containsKey(nodeGroupMemberships_, nodeId) && inet::contains(nodeGroupMemberships_[nodeId], multicastDestId);
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
    bool oldCellIsGNodeB = isGNodeB(oldCell);
    bool newCellIsGNodeB = isGNodeB(newCell);

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
    if (oldCellIsGNodeB == newCellIsGNodeB) {
        addUeCollectorToEnodeB(ue, ueColl, newCell);
    }
    else {
        if (newCellIsGNodeB) {
            // Retrieve NrUeCollector
            cModule *ueModule = getModuleByMacNodeId(ue);
            if (ueModule->findSubmodule("nrUeCollector") == -1)
                ueColl = check_and_cast<UeStatsCollector *>(ueModule->getSubmodule("nrUeCollector"));
            else
                throw cRuntimeError("LteBinder::moveUeCollector - Ue [%hu] does not have an 'nrUeCollector' submodule required for the gNB", num(ue));
            addUeCollectorToEnodeB(ue, ueColl, newCell);
        }
        else {
            // Retrieve ueCollector for eNodeB
            cModule *ueModule = getModuleByMacNodeId(ue);
            if (ueModule->findSubmodule("ueCollector") == -1)
                ueColl = check_and_cast<UeStatsCollector *>(ueModule->getSubmodule("ueCollector"));
            else
                throw cRuntimeError("LteBinder::moveUeCollector - Ue [%hu] does not have an 'ueCollector' submodule required for the eNB", num(ue));
            addUeCollectorToEnodeB(ue, ueColl, newCell);
        }
    }
}

bool Binder::isGNodeB(MacNodeId enbId)
{
    ASSERT(getNodeTypeById(enbId) == NODEB);
    cModule *module = getModuleByMacNodeId(enbId);
    std::string nodeTypeStr = module->par("nodeType").stdstringValue();
    return nodeTypeStr == "GNODEB";
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
        return module->getSubmodule("cellularNic")->getSubmodule("nrRlc")->getSubmodule("entityManager");
    return module->getSubmodule("cellularNic")->getSubmodule("rlc")->getSubmodule("entityManager");
}

cModule *Binder::getPdcpByNodeId(MacNodeId nodeId)
{
    // UE might have left the simulation, return NULL in this case
    // since we do not have a MAC-Module anymore
    cModule *module = getNodeModule(nodeId);
    if (module == nullptr) {
        return nullptr;
    }
    cModule *pdcp = module->getSubmodule("cellularNic")->getSubmodule("pdcp");
    // If pdcp is a compound module (PdcpLayer), return its entityManager submodule
    cModule *mgr = pdcp->getSubmodule("entityManager");
    return mgr ? mgr : pdcp;
}

MacNodeId Binder::getOrAssignDestIdForMulticastAddress(inet::Ipv4Address multicastAddr)
{
    if (inet::containsKey(multicastAddrToDestId_, multicastAddr))
        return multicastAddrToDestId_[multicastAddr];

    // Allocate new ID
    MacNodeId newDestId = MacNodeId(multicastDestIdCounter_++);
    multicastAddrToDestId_[multicastAddr] = newDestId;
    multicastDestIdToAddr_[newDestId] = multicastAddr;

    EV << "Binder::allocateMulticastDestId - allocated multicast destination ID " << newDestId << " for multicast address " << multicastAddr << endl;

    return newDestId;
}

MacNodeId Binder::getDestIdForMulticastAddress(inet::Ipv4Address multicastAddr)
{
    if (!inet::containsKey(multicastAddrToDestId_, multicastAddr))
        throw cRuntimeError("Binder::getDestIdForMulticastAddress - no destination ID allocated for multicast address %s", multicastAddr.str().c_str());
    return multicastAddrToDestId_[multicastAddr];
}

inet::Ipv4Address Binder::getAddressForMulticastDestId(MacNodeId multicastDestId)
{
    if (!inet::containsKey(multicastDestIdToAddr_, multicastDestId))
        throw cRuntimeError("Binder::getAddressForMulticastDestId - no address allocated for multicast destination ID %hu", num(multicastDestId));
    return multicastDestIdToAddr_[multicastDestId];
}

cModule *Binder::getRrcByNodeId(MacNodeId nodeId)
{
    cModule *module = getNodeModule(nodeId);
    if (module == nullptr) {
        return nullptr;
    }
    return module->getSubmodule("cellularNic")->getSubmodule("rrc");
}

bool Binder::isDualConnectivityRequired(FlowControlInfo *info)
{
    MacNodeId sourceId = info->getSourceId();
    MacNodeId destId = info->getDestId();

    // Part 1: Check if NodeB is in DC setup
    MacNodeId nodeB = (getNodeTypeById(sourceId) == UE) ? getServingNode(sourceId) : sourceId;
    ASSERT(nodeB != NODEID_NONE);

    MacNodeId secondaryNode = getSecondaryNode(nodeB);
    MacNodeId masterNode = getMasterNodeOrSelf(nodeB);
    bool nodeBInDC = (secondaryNode != NODEID_NONE) || (masterNode != nodeB);

    // Part 2: Check if UE is dual technology capable
    MacNodeId ue = getNodeTypeById(sourceId) == UE ? sourceId :
                   getNodeTypeById(destId) == UE ? destId :
                   NODEID_NONE;

    bool ueIsDualTech = false;  //TODO true? if a nodeB in DC setup sends multicast, can it use dual connectivity?
    if (ue != NODEID_NONE) {
        Rrc *rrc = check_and_cast<Rrc*>(getRrcByNodeId(ue));
        ueIsDualTech = rrc->isDualTechnology();
    }

    return nodeBInDC && ueIsDualTech;
}

void Binder::establishUnidirectionalDataConnection(FlowControlInfo *info)
{
    bool dualConnected = isDualConnectivityRequired(info);
    if (!dualConnected) {
        createConnection(info, true);
    }
    else {
        MacNodeId sourceId = info->getSourceId();
        MacNodeId destId = info->getDestId();
        bool isMulticast = info->getMulticastGroupId() != NODEID_NONE;

        // Get UE RRC if any endpoint is UE
        Rrc *ueRrc = (getNodeTypeById(sourceId) == UE) ? check_and_cast<Rrc*>(getRrcByNodeId(sourceId)) :
                     (!isMulticast && getNodeTypeById(destId) == UE) ? check_and_cast<Rrc*>(getRrcByNodeId(destId)) :
                     nullptr;

        //TODO assert that master is LTE, and secondary is NT;   alternatively, choose the UE nodeId that matches the technology of the NODEB

        // LTE Connection (Master)
        FlowControlInfo lteInfo = *info;
        lteInfo.setSourceId(getNodeTypeById(sourceId) == UE ?
                            ueRrc->getLteNodeId() :
                            getMasterNodeOrSelf(sourceId));
        if (!isMulticast) {  // Only set destId for unicast
            lteInfo.setDestId(getNodeTypeById(destId) == UE ?
                              ueRrc->getLteNodeId() :
                              getMasterNodeOrSelf(destId));
        }
        createConnection(&lteInfo, true);

        // NR Connection (Secondary)
        FlowControlInfo nrInfo = *info;
        nrInfo.setSourceId(getNodeTypeById(sourceId) == UE ?
                           ueRrc->getNrNodeId() :
                           getSecondaryNode(getMasterNodeOrSelf(sourceId)));
        if (!isMulticast) {  // Only set destId for unicast
            nrInfo.setDestId(getNodeTypeById(destId) == UE ?
                             ueRrc->getNrNodeId() :
                             getSecondaryNode(getMasterNodeOrSelf(destId)));
        }
        createConnection(&nrInfo, false);
    }
}

void Binder::createConnection(FlowControlInfo *lteInfo, bool withPdcp)
{
    MacNodeId sourceId = lteInfo->getSourceId();
    MacNodeId destId = lteInfo->getDestId();
    MacNodeId groupId = lteInfo->getMulticastGroupId();

    EV << "Binder::establishUnidirectionalDataConnection - establishing connection from sourceId=" << sourceId
       << " to destId=" << destId << " groupId=" << groupId << endl;

    bool sourceIsEnb = getNodeTypeById(sourceId) == NODEB;
    bool destIsEnb = getNodeTypeById(destId) == NODEB;
    ASSERT(!sourceIsEnb || !destIsEnb);  // they cannot be both NodeBs

    createOutgoingConnectionOnNode(sourceId, lteInfo, getNodeTypeById(sourceId)==UE || withPdcp);

    if (groupId == NODEID_NONE) {
        createIncomingConnectionOnNode(destId, lteInfo, getNodeTypeById(destId)==UE || withPdcp);
    }
    else {
        for (auto& [nodeId,_] : getNodeInfoMap())  //TODO use lte ones if LTE in DC setup, and NR ones if NR in DC setup
            if (nodeId != sourceId && isInMulticastGroup(nodeId, groupId))
                createIncomingConnectionOnNode(nodeId, lteInfo, getNodeTypeById(nodeId)==UE || withPdcp);
    }
}


void Binder::createIncomingConnectionOnNode(MacNodeId nodeId, FlowControlInfo *lteInfo, bool withPdcp)
{
    Rrc *rrc = check_and_cast<Rrc*>(getRrcByNodeId(nodeId));
    rrc->createIncomingConnection(lteInfo, withPdcp);
}

void Binder::createOutgoingConnectionOnNode(MacNodeId nodeId, FlowControlInfo *lteInfo, bool withPdcp)
{
    Rrc *rrc = check_and_cast<Rrc*>(getRrcByNodeId(nodeId));
    rrc->createOutgoingConnection(lteInfo, withPdcp);
}

} //namespace
