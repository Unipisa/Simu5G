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

#include "common/binder/Binder.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include <cctype>
#include <algorithm>

using namespace std;
using namespace inet;

Define_Module(Binder);

void Binder::registerCarrier(double carrierFrequency, unsigned int carrierNumBands, unsigned int numerologyIndex, bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl)
{
    CarrierInfoMap::iterator it = componentCarriers_.find(carrierFrequency);
    if (it != componentCarriers_.end() && carrierNumBands <= componentCarriers_[carrierFrequency].numBands)
    {
        EV << "Binder::registerCarrier - Carrier @ " << carrierFrequency << "GHz already registered" << endl;
    }
    else
    {
        CarrierInfo cInfo;
        cInfo.carrierFrequency = carrierFrequency;
        cInfo.numBands = carrierNumBands;
        cInfo.numerologyIndex = numerologyIndex;
        cInfo.slotFormat = computeSlotFormat(useTdd,tddNumSymbolsDl,tddNumSymbolsUl);
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

void Binder::registerCarrierUe(double carrierFrequency, unsigned int numerologyIndex, MacNodeId ueId)
{
    // check if carrier exists in the system
    CarrierUeMap::iterator it = carrierUeMap_.find(carrierFrequency);
    if (it == carrierUeMap_.end())
        throw cRuntimeError("Binder::registerCarrierUe - Carrier [%fGHz] not found", carrierFrequency);

    carrierUeMap_[carrierFrequency].insert(ueId);

    if (ueNumerologyIndex_.find(ueId) == ueNumerologyIndex_.end())
    {
        std::set<NumerologyIndex> numerologySet;
        ueNumerologyIndex_[ueId] = numerologySet;
    }
    ueNumerologyIndex_[ueId].insert(numerologyIndex);

    if (ueMaxNumerologyIndex_.size() <= ueId)
    {
        ueMaxNumerologyIndex_.resize(ueId + 1);
        ueMaxNumerologyIndex_[ueId] = numerologyIndex;
    }
    else
        ueMaxNumerologyIndex_[ueId] = (numerologyIndex > ueMaxNumerologyIndex_[ueId]) ? numerologyIndex : ueMaxNumerologyIndex_[ueId];
}

const UeSet& Binder::getCarrierUeSet(double carrierFrequency)
{
    CarrierUeMap::iterator it = carrierUeMap_.find(carrierFrequency);
    if (it == carrierUeMap_.end())
        throw cRuntimeError("Binder::getCarrierUeSet - Carrier [%fGHz] not found", carrierFrequency);

    return carrierUeMap_[carrierFrequency];
}

NumerologyIndex Binder::getUeMaxNumerologyIndex(MacNodeId ueId)
{
    return ueMaxNumerologyIndex_[ueId];
}

const std::set<NumerologyIndex>* Binder::getUeNumerologyIndex(MacNodeId ueId)
{
    if (ueNumerologyIndex_.find(ueId) == ueNumerologyIndex_.end())
        return NULL;
    return &ueNumerologyIndex_[ueId];
}

SlotFormat Binder::computeSlotFormat(bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl)
{
    SlotFormat sf;
    if (!useTdd)
    {
        sf.tdd = false;

        // these values are not used when tdd is false
        sf.numDlSymbols = 0;
        sf.numUlSymbols = 0;
        sf.numFlexSymbols = 0;
    }
    else
    {
        unsigned int rbxDl = 7;  // TODO replace with the parameter obtained from NED file once you moved the function to the Component Carrier

        sf.tdd = true;
        unsigned int numSymbols = rbxDl * 2;

        if (tddNumSymbolsDl+tddNumSymbolsUl > numSymbols)
            throw cRuntimeError("Binder::computeSlotFormat - Number of symbols not valid - DL[%d] UL[%d]", tddNumSymbolsDl,tddNumSymbolsUl);

        sf.numDlSymbols = tddNumSymbolsDl;
        sf.numUlSymbols = tddNumSymbolsUl;
        sf.numFlexSymbols = numSymbols - tddNumSymbolsDl - tddNumSymbolsUl;
    }
    return sf;
}

SlotFormat Binder::getSlotFormat(double carrierFrequency)
{
    CarrierInfoMap::iterator it = componentCarriers_.find(carrierFrequency);
    if (it == componentCarriers_.end())
        throw cRuntimeError("Binder::getSlotFormat - Carrier [%fGHz] not found", carrierFrequency);

    return it->second.slotFormat;
}


void Binder::unregisterNode(MacNodeId id)
{
    EV << NOW << " Binder::unregisterNode - unregistering node " << id << endl;

    std::map<Ipv4Address, MacNodeId>::iterator it;
    for(it = macNodeIdToIPAddress_.begin(); it != macNodeIdToIPAddress_.end(); )
    {
        if(it->second == id)
        {
            macNodeIdToIPAddress_.erase(it++);
        }
        else
        {
            it++;
        }
    }

    // iterate all nodeIds and find HarqRx buffers dependent on 'id'
    std::map<int, OmnetId>::iterator idIter;
    for (idIter = nodeIds_.begin(); idIter != nodeIds_.end(); idIter++){
        LteMacBase* mac = getMacFromMacNodeId(idIter->first);
        mac->unregisterHarqBufferRx(id);
    }

    // remove 'id' from LteMacBase* cache but do not delte pointer.
    if(macNodeIdToModule_.erase(id) != 1){
        EV_ERROR << "Cannot unregister node - node id \"" << id << "\" - not found";
    }

    // remove 'id' from MacNodeId mapping
    if(nodeIds_.erase(id) != 1){
        EV_ERROR << "Cannot unregister node - node id \"" << id << "\" - not found";
    }
}

MacNodeId Binder::registerNode(cModule *module, RanNodeType type, MacNodeId masterId, bool registerNr)
{
    Enter_Method_Silent("registerNode");

    MacNodeId macNodeId = -1;

    if (type == UE)
    {
        if (!registerNr)
            macNodeId = macNodeIdCounter_[1]++;
        else
            macNodeId = macNodeIdCounter_[2]++;
    }
    else if (type == ENODEB || type == GNODEB)
    {
        macNodeId = macNodeIdCounter_[0]++;
    }

    EV << "Binder : Assigning to module " << module->getName()
       << " with OmnetId: " << module->getId() << " and MacNodeId " << macNodeId
       << "\n";

    // registering new node to Binder

    nodeIds_[macNodeId] = module->getId();

    if (!registerNr)
        module->par("macNodeId") = macNodeId;
    else
        module->par("nrMacNodeId") = macNodeId;

    if (type == UE)
    {
        registerNextHop(masterId, macNodeId);
    }
    else if (type == ENODEB || type == GNODEB)
    {
        module->par("macCellId") = macNodeId;
        registerNextHop(macNodeId, macNodeId);
        registerMasterNode(masterId, macNodeId);
    }
    return macNodeId;
}

void Binder::registerNextHop(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method_Silent("registerNextHop");

    EV << "Binder : Registering slave " << slaveId << " to master "
       << masterId << "\n";

    if (masterId != slaveId)
    {
        dMap_[masterId][slaveId] = true;
    }

    if (nextHop_.size() <= slaveId)
        nextHop_.resize(slaveId + 1);
    nextHop_[slaveId] = masterId;
}

void Binder::registerMasterNode(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method_Silent("registerMasterNode");
    EV << "Binder : Registering slave " << slaveId << " to master "
       << masterId << "\n";

    if (secondaryNodeToMasterNode_.size() <= slaveId)
        secondaryNodeToMasterNode_.resize(slaveId + 1);

    if (masterId == 0)  // this node is a master itself
        masterId = slaveId;
    secondaryNodeToMasterNode_[slaveId] = masterId;
}

void Binder::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
        phyPisaData.setBlerShift(par("blerShift"));

    if (stage == inet::INITSTAGE_LAST)
    {
        // if avg interference enabled, compute CQIs
        computeAverageCqiForBackgroundUes();
    }

}

void Binder::unregisterNextHop(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method_Silent("unregisterNextHop");
    EV << "Binder : Unregistering slave " << slaveId << " from master "
       << masterId << "\n";
    dMap_[masterId][slaveId] = false;

    if (nextHop_.size() <= slaveId)
        return;
    nextHop_[slaveId] = 0;
}

OmnetId Binder::getOmnetId(MacNodeId nodeId)
{
    std::map<int, OmnetId>::iterator it = nodeIds_.find(nodeId);
    if(it != nodeIds_.end())
        return it->second;
    return 0;
}

std::map<int, OmnetId>::const_iterator Binder::getNodeIdListBegin()
{
    return nodeIds_.begin();
}

std::map<int, OmnetId>::const_iterator Binder::getNodeIdListEnd()
{
    return nodeIds_.end();
}

MacNodeId Binder::getMacNodeIdFromOmnetId(OmnetId id){
	std::map<int, OmnetId>::iterator it;
	for (it = nodeIds_.begin(); it != nodeIds_.end(); ++it )
	    if (it->second == id)
	        return it->first;
	return 0;
}

LteMacBase* Binder::getMacFromMacNodeId(MacNodeId id)
{
    if (id == 0)
        return nullptr;

    LteMacBase* mac;
    if (macNodeIdToModule_.find(id) == macNodeIdToModule_.end())
    {
        mac = check_and_cast<LteMacBase*>(getMacByMacNodeId(id));
        macNodeIdToModule_[id] = mac;
    }
    else
    {
        mac = macNodeIdToModule_[id];
    }
    return mac;
}

MacNodeId Binder::getNextHop(MacNodeId slaveId)
{
    Enter_Method_Silent("getNextHop");
    if (slaveId >= nextHop_.size())
        throw cRuntimeError("Binder::getNextHop(): bad slave id %d", slaveId);
    return nextHop_[slaveId];
}

MacNodeId Binder::getMasterNode(MacNodeId slaveId)
{
    Enter_Method_Silent("getMasterNode");
    if (slaveId >= secondaryNodeToMasterNode_.size())
        throw cRuntimeError("Binder::getMasterNode(): bad slave id %d", slaveId);
    return secondaryNodeToMasterNode_[slaveId];
}

void Binder::registerName(MacNodeId nodeId, const char* moduleName)
{
    int len = strlen(moduleName);
    macNodeIdToModuleName_[nodeId] = new char[len+1];
    strcpy(macNodeIdToModuleName_[nodeId], moduleName);
}

const char* Binder::getModuleNameByMacNodeId(MacNodeId nodeId)
{
    if (macNodeIdToModuleName_.find(nodeId) == macNodeIdToModuleName_.end())
        throw cRuntimeError("Binder::getModuleNameByMacNodeId - node ID not found");
    return macNodeIdToModuleName_[nodeId];
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
    if (lastUplinkTransmission_ < NOW - 2*TTI)
    {
        // data structures have not been used in the last 2 time slots,
        // so they do not need to be updated.
        return;
    }

    UplinkTransmissionMap::iterator it = ulTransmissionMap_.begin();
    for (; it != ulTransmissionMap_.end(); ++it)
    {
        // the second element (i.e. referring to the old time slot) becomes the first element
        if (!(it->second.empty()))
            it->second.erase(it->second.begin());
    }
    lastUpdateUplinkTransmissionInfo_ = NOW;
}

void Binder::storeUlTransmissionMap(double carrierFreq, Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, LtePhyBase* phy, Direction dir)
{
    UeAllocationInfo info;
    info.nodeId = nodeId;
    info.cellId = cellId;
    info.phy = phy;
    info.dir = dir;
    info.trafficGen = nullptr;

    if (ulTransmissionMap_.find(carrierFreq) == ulTransmissionMap_.end() || ulTransmissionMap_[carrierFreq].size() == 0)
    {
        int numCarrierBands = componentCarriers_[carrierFreq].numBands;
        ulTransmissionMap_[carrierFreq].resize(2);
        ulTransmissionMap_[carrierFreq][PREV_TTI].resize(numCarrierBands);
        ulTransmissionMap_[carrierFreq][CURR_TTI].resize(numCarrierBands);
    }
    else if (ulTransmissionMap_[carrierFreq].size() == 1)
    {
        int numCarrierBands = componentCarriers_[carrierFreq].numBands;
        ulTransmissionMap_[carrierFreq].push_back(std::vector<std::vector<UeAllocationInfo> >());
        ulTransmissionMap_[carrierFreq][CURR_TTI].resize(numCarrierBands);
    }

    // for each allocated band, store the UE info
    std::map<Band, unsigned int>::iterator it = rbMap[antenna].begin(), et = rbMap[antenna].end();
    for ( ; it != et; ++it)
    {
        Band b = it->first;
        if (it->second > 0)
            ulTransmissionMap_[carrierFreq][CURR_TTI][b].push_back(info);
    }

    lastUplinkTransmission_ = NOW;
}

void Binder::storeUlTransmissionMap(double carrierFreq, Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, TrafficGeneratorBase* trafficGen, Direction dir)
{
    UeAllocationInfo info;
    info.nodeId = nodeId;
    info.cellId = cellId;
    info.phy = nullptr;
    info.dir = dir;
    info.trafficGen = trafficGen;

    if (ulTransmissionMap_.find(carrierFreq) == ulTransmissionMap_.end() || ulTransmissionMap_[carrierFreq].size() == 0)
    {
        int numCarrierBands = componentCarriers_[carrierFreq].numBands;
        ulTransmissionMap_[carrierFreq].resize(2);
        ulTransmissionMap_[carrierFreq][PREV_TTI].resize(numCarrierBands);
        ulTransmissionMap_[carrierFreq][CURR_TTI].resize(numCarrierBands);
    }
    else if (ulTransmissionMap_[carrierFreq].size() == 1)
    {
        int numCarrierBands = componentCarriers_[carrierFreq].numBands;
        ulTransmissionMap_[carrierFreq].push_back(std::vector<std::vector<UeAllocationInfo> >());
        ulTransmissionMap_[carrierFreq][CURR_TTI].resize(numCarrierBands);
    }

    // for each allocated band, store the UE info
    std::map<Band, unsigned int>::iterator it = rbMap[antenna].begin(), et = rbMap[antenna].end();
    for ( ; it != et; ++it)
    {
        Band b = it->first;
        if (it->second > 0)
            ulTransmissionMap_[carrierFreq][CURR_TTI][b].push_back(info);
    }

    lastUplinkTransmission_ = NOW;
}


const std::vector<std::vector<UeAllocationInfo> >* Binder::getUlTransmissionMap(double carrierFreq, UlTransmissionMapTTI t)
{
    if (ulTransmissionMap_.find(carrierFreq) == ulTransmissionMap_.end())
        return NULL;
    return &(ulTransmissionMap_[carrierFreq][t]);
}

void Binder::registerX2Port(X2NodeId nodeId, int port)
{
    if (x2ListeningPorts_.find(nodeId) == x2ListeningPorts_.end() )
    {
        // no port has yet been registered
        std::list<int> ports;
        ports.push_back(port);
        x2ListeningPorts_[nodeId] = ports;
    }
    else
    {
        x2ListeningPorts_[nodeId].push_back(port);
    }
}

int Binder::getX2Port(X2NodeId nodeId)
{
    if (x2ListeningPorts_.find(nodeId) == x2ListeningPorts_.end() )
        throw cRuntimeError("Binder::getX2Port - No ports available on node %d", nodeId);

    int port = x2ListeningPorts_[nodeId].front();
    x2ListeningPorts_[nodeId].pop_front();
    return port;
}

Cqi Binder::meanCqi(std::vector<Cqi> bandCqi,MacNodeId id,Direction dir)
{
    std::vector<Cqi>::iterator it;
    Cqi mean=0;
    for (it=bandCqi.begin();it!=bandCqi.end();++it)
    {
        mean+=*it;
    }
    mean/=bandCqi.size();

    if(mean==0)
        mean = 1;

    return mean;
}

Cqi Binder::medianCqi(std::vector<Cqi> bandCqi,MacNodeId id,Direction dir)
{
    std::sort(bandCqi.begin(),bandCqi.end());

    int medianPoint = bandCqi.size()/2;

    EV << "Binder::medianCqi - median point is " << bandCqi.size() << "/2 = " << medianPoint << ". MedianCqi = " << bandCqi[medianPoint] << endl;

    return bandCqi[medianPoint];

}

bool Binder::checkD2DCapability(MacNodeId src, MacNodeId dst)
{
    if (src < UE_MIN_ID || (src >= macNodeIdCounter_[1] && src < NR_UE_MIN_ID) || src >= macNodeIdCounter_[2]
            || dst < UE_MIN_ID || (dst >= macNodeIdCounter_[1] && dst < NR_UE_MIN_ID) || dst >= macNodeIdCounter_[2])
        throw cRuntimeError("Binder::checkD2DCapability - Node Id not valid. Src %d Dst %d", src, dst);

    // if the entry is missing, check if the receiver is D2D capable and update the map
    if (d2dPeeringMap_.find(src) == d2dPeeringMap_.end() || d2dPeeringMap_[src].find(dst) == d2dPeeringMap_[src].end())
    {
        LteMacBase* dstMac = getMacFromMacNodeId(dst);
        if (dstMac->isD2DCapable())
        {
            // set the initial mode
            if (nextHop_[src] == nextHop_[dst])
            {
                // if served by the same cell, then the mode is selected according to the corresponding parameter
                LteMacBase* srcMac = getMacFromMacNodeId(src);
                bool d2dInitialMode = srcMac->getAncestorPar("d2dInitialMode").boolValue();
                d2dPeeringMap_[src][dst] = (d2dInitialMode) ? DM : IM;
            }
            else
            {
                // if served by different cells, then the mode can be IM only
                d2dPeeringMap_[src][dst] = IM;
            }

            EV << "Binder::checkD2DCapability - UE " << src << " may transmit to UE " << dst << " using D2D (current mode " << ((d2dPeeringMap_[src][dst] == DM) ? "DM)" : "IM)") << endl;

            // this is a D2D-capable flow
            return true;
        }
        else
        {
            EV << "Binder::checkD2DCapability - UE " << src << " may not transmit to UE " << dst << " using D2D (UE " << dst << " is not D2D capable)" << endl;
            // this is not a D2D-capable flow
            return false;
        }

    }

    // an entry is present, hence this is a D2D-capable flow
    return true;
}

bool Binder::getD2DCapability(MacNodeId src, MacNodeId dst)
{
    if (src < UE_MIN_ID || (src >= macNodeIdCounter_[1] && src < NR_UE_MIN_ID) || src >= macNodeIdCounter_[2]
            || dst < UE_MIN_ID || (dst >= macNodeIdCounter_[1] && dst < NR_UE_MIN_ID) || dst >= macNodeIdCounter_[2])
        throw cRuntimeError("Binder::getD2DCapability - Node Id not valid. Src %d Dst %d", src, dst);

    // if the entry is missing, returns false
    if (d2dPeeringMap_.find(src) == d2dPeeringMap_.end() || d2dPeeringMap_[src].find(dst) == d2dPeeringMap_[src].end())
        return false;

    // the entry exists, no matter if it is DM or IM
    return true;
}

std::map<MacNodeId, std::map<MacNodeId, LteD2DMode> >* Binder::getD2DPeeringMap()
{
    return &d2dPeeringMap_;
}

LteD2DMode Binder::getD2DMode(MacNodeId src, MacNodeId dst)
{
    if (src < UE_MIN_ID || (src >= macNodeIdCounter_[1] && src < NR_UE_MIN_ID) || src >= macNodeIdCounter_[2]
            || dst < UE_MIN_ID || (dst >= macNodeIdCounter_[1] && dst < NR_UE_MIN_ID) || dst >= macNodeIdCounter_[2])
        throw cRuntimeError("Binder::getD2DMode - Node Id not valid. Src %d Dst %d", src, dst);

    return d2dPeeringMap_[src][dst];
}

bool Binder::isFrequencyReuseEnabled(MacNodeId nodeId)
{
    // a d2d-enabled UE can use frequency reuse if it can communicate using DM with all its peers
    // in fact, the scheduler does not know to which UE it will communicate when it grants some RBs
    if (d2dPeeringMap_.find(nodeId) == d2dPeeringMap_.end())
        return false;

    std::map<MacNodeId, LteD2DMode>::iterator it = d2dPeeringMap_[nodeId].begin();
    if (it == d2dPeeringMap_[nodeId].end())
        return false;

    for (; it != d2dPeeringMap_[nodeId].end(); ++it)
    {
        if (it->second == IM)
            return false;
    }
    return true;
}


void Binder::registerMulticastGroup(MacNodeId nodeId, int32_t groupId)
{
    if (multicastGroupMap_.find(nodeId) == multicastGroupMap_.end())
    {
        MulticastGroupIdSet newSet;
        newSet.insert(groupId);
        multicastGroupMap_[nodeId] = newSet;
    }
    else
    {
        multicastGroupMap_[nodeId].insert(groupId);
    }
}

bool Binder::isInMulticastGroup(MacNodeId nodeId, int32_t groupId)
{
    if (multicastGroupMap_.find(nodeId) == multicastGroupMap_.end())
        return false;   // the node is not enrolled in any group
    if (multicastGroupMap_[nodeId].find(groupId) == multicastGroupMap_[nodeId].end())
        return false;   // the node is not enrolled in the given group

    return true;
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
    std::vector<UeInfo*>::iterator it = ueList_.begin();
    for (; it != ueList_.end(); ++it)
    {
        if ((*it)->id == id)
        {
            (*it)->cellId = newCellId;
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
    if (ueHandoverTriggered_.find(nodeId) == ueHandoverTriggered_.end())
        return false;
    return true;
}

void Binder::removeUeHandoverTriggered(MacNodeId nodeId)
{
    ueHandoverTriggered_.erase(nodeId);
}

void Binder::addHandoverTriggered(MacNodeId nodeId, MacNodeId srcId, MacNodeId destId)
{
    std::pair<MacNodeId, MacNodeId> p(srcId, destId);
    handoverTriggered_[nodeId] = p;
}

const std::pair<MacNodeId, MacNodeId>* Binder::getHandoverTriggered(MacNodeId nodeId)
{
    if (handoverTriggered_.find(nodeId) == handoverTriggered_.end())
        return NULL;
    return &handoverTriggered_[nodeId];
}

void Binder::removeHandoverTriggered(MacNodeId nodeId)
{
    std::map<MacNodeId, std::pair<MacNodeId, MacNodeId> >::iterator it = handoverTriggered_.find(nodeId);
    if (it!=handoverTriggered_.end())
        handoverTriggered_.erase(it);
}


void Binder::computeAverageCqiForBackgroundUes()
{
    EV << " ===== Binder::computeAverageCqiForBackgroundUes - START =====" << endl;

    // initialize interference matrix
    for (unsigned int i=0; i<bgTrafficManagerList_.size(); i++)
    {
        std::map<unsigned int, double> tmp;
        for (unsigned int j=0; j<bgTrafficManagerList_.size(); j++)
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
     * 2) while average interference variation between 2 consecutives steps is above a certain threshold
     *  - Update interference
     *  - Compute SINR with interference
     *  - Update cell block usage according to connect UEs
     */
    while(condition)
    {
        countInterferenceCheck++;
        EV << " * ITERATION " <<  countInterferenceCheck << " *" << endl;

        // --- MAIN Interference Check Cycle --- //

        // loop through the BackgroundTrafficManagers (one per cell)
        for (unsigned int bgTrafficManagerId = 0; bgTrafficManagerId < bgTrafficManagerList_.size(); bgTrafficManagerId++)
        {
            BgTrafficManagerInfo* info = bgTrafficManagerList_.at(bgTrafficManagerId);
            if (!(info->init))
                continue;

            BackgroundTrafficManager* bgTrafficManager = info->bgTrafficManager;
            unsigned int numBands = bgTrafficManager->getNumBands();

            //---------------------------------------------------------------------
            // STEP 1: update mutual interference
            // for iterations after the first one, update the interference before analyzing a whole cell
            if (countInterferenceCheck>1)
                updateMutualInterference(bgTrafficManagerId, numBands);

            double cellRbsDl = 0;
            double cellRbsUl = 0;

            // Compute the SINR for each UE within the cell
            auto bgUes_it = bgTrafficManager->getBgUesBegin();
            auto bgUes_et = bgTrafficManager->getBgUesEnd();
            int cont = 0;
            while (bgUes_it != bgUes_et)
            {
                TrafficGeneratorBase* bgUe = *bgUes_it;

                // for the first iteration, update interference before analyzing each UE
                if(countInterferenceCheck==1)
                    updateMutualInterference(bgTrafficManagerId, numBands);


                //---------------------------------------------------------------------
                // STEP 2: compute SINR
                bool losStatus = false;
                inet::Coord bsCoord = bgTrafficManager->getBsCoord();
                inet::Coord bgUeCoord = bgUe->getCoord();
                double bsTxPower = bgTrafficManager->getBsTxPower();
                double bgUeTxPower = bgUe->getTxPwr();

                double sinrDl = computeSinr(bgTrafficManagerId, bsTxPower, bsCoord, bgUeCoord, DL, losStatus);
//                double sinrUl = computeSinr(bgTrafficManagerId, bgUeTxPower, bgUeCoord, bsCoord, UL, losStatus);

                // update CQI for the bg UE
                bgUe->setCqiFromSinr(sinrDl, DL);
                bgUe->setCqiFromSinr(30.0, UL);   // TEMPORARY HACK

                //---------------------------------------------------------------------
                // STEP 3: update block allocation
                // obtain UE load request and convert it to rbs based on SINR
                double ueLoadDl = bgUe->getAvgLoad(DL);
//                double ueLoadUl = bgUe->getAvgLoad(UL);
                double ueRbsDl = computeRequestedRbsFromSinr(sinrDl,ueLoadDl);
//                double ueRbsUl = computeRequestedRbsFromSinr(sinrUl,ueLoadUl);
                double ueRbsUl = 0;

                EV << "BgTrafficManager " << bgTrafficManagerId << " - UE[" << cont << "] sinrDl[" << sinrDl << "] load[" << ueLoadDl << "] rbsDl[" << ueRbsDl << "]" << endl;

                if(ueRbsDl<0 || ueRbsUl<0)
                {
                    EV << "Error! Computed negative requested rbs DL[" << ueRbsDl << "] UL[" << ueRbsUl << "]" << endl;
                    throw cRuntimeError("Binder::computeAverageCqiForBackgroundUes - Error! Computed negative requested rbs\n");
                }

                double availableBandsDl = numBands - cellRbsDl;
//                unsigned int availableBandsUl = numBands - cellRbsUl;

                // check if there is room for ueRbs
                ueRbsDl = (ueRbsDl > availableBandsDl) ? availableBandsDl : ueRbsDl;
//                ueRbsUl = (ueRbsUl > availableBandsUl) ? availableBandsUl : ueRbsUl;

                cellRbsDl += ueRbsDl;
//                cellRbsUl += ueRbsUl;

                if (countInterferenceCheck == 1)
                {
                    // update allocation elem for this background traffic manager
                    info->allocatedRbs[DL] = cellRbsDl;
//                    info->allocatedRbs[UL] = cellRbsUl;

                    EV << "BgTrafficManager " << bgTrafficManagerId << " - allocatedRbsDl[" << info->allocatedRbs[DL] << "]" << endl;

                }

                ++bgUes_it;
                ++cont;
            }

            if (countInterferenceCheck > 1)
            {
                // update allocation elem for this background traffic manager
                info->allocatedRbs[DL] = cellRbsDl;
//                info->allocatedRbs[UL] = cellRbsUl;

                EV << "BgTrafficManager " << bgTrafficManagerId << ": allocatedRbsDl[" << info->allocatedRbs[DL] << "]" << endl;
            }
        }

        EV << "* END ITERATION " << countInterferenceCheck << endl;
        if ( countInterferenceCheck > MAX_INTERFERENCE_CHECK )
            condition = false;
    }

    // DEBUG
    // loop through the BackgroundTrafficManagers (one per cell)
    for (unsigned int bgTrafficManagerId = 0; bgTrafficManagerId < bgTrafficManagerList_.size(); bgTrafficManagerId++)
    {
        BgTrafficManagerInfo* info = bgTrafficManagerList_.at(bgTrafficManagerId);
        if (!(info->init))
            continue;

        BackgroundTrafficManager* bgTrafficManager = info->bgTrafficManager;

        // Compute the SINR for each UE within the cell
        auto bgUes_it = bgTrafficManager->getBgUesBegin();
        auto bgUes_et = bgTrafficManager->getBgUesEnd();
        int cont = 0;
        while (bgUes_it != bgUes_et)
        {
            TrafficGeneratorBase* bgUe = *bgUes_it;
            Cqi cqiDl = bgUe->getCqi(DL);
            Cqi cqiUl = bgUe->getCqi(UL);

            std::cout << "BgTrafficManager " << bgTrafficManagerId << " - UE[" << cont << "] cqiDl[" << cqiDl << "] cqiUl[" << cqiUl << "] "<< endl;
            ++bgUes_it;
            ++cont;
        }
    }

    EV << " ===== Binder::computeAverageCqiForBackgroundUes - END =====" << endl;

}

void Binder::updateMutualInterference(unsigned int bgTrafficManagerId, unsigned int numBands)
{
    EV << "Binder::updateMutualInterference - computing interference for traffic manager " << bgTrafficManagerId << endl;

    double ownRbs, extRbs; // current rbs allocation

    BgTrafficManagerInfo* ownInfo = bgTrafficManagerList_.at(bgTrafficManagerId);

    // obtain current allocation info
    ownRbs = ownInfo->allocatedRbs[DL];

    // loop through the BackgroundTrafficManagers (one per cell)
    for (unsigned int extId = 0; extId < bgTrafficManagerList_.size(); extId++)
    {
        // skip own interference
        if (extId == bgTrafficManagerId)
            continue;

        // obtain current allocation info for interfering bg cell
        BgTrafficManagerInfo* extInfo = bgTrafficManagerList_.at(extId);
        extRbs = extInfo->allocatedRbs[DL];

        double newOverlapPercentage = computeInterferencePercentage(ownRbs, extRbs, numBands);

        EV << "ownId[" << bgTrafficManagerId << "] extId[" << extId << "] - ownRbs[" << ownRbs << "] extRbs[" << extRbs << "] - overlap[" << newOverlapPercentage << "]" << endl;


        // update interference
        bgCellsInterferenceMatrix_[bgTrafficManagerId][extId] = newOverlapPercentage;

    }// end ext-cell computation
}

double Binder::computeInterferencePercentage(double n, double k, unsigned int numBands)
{
//    if (allocationType_ == FIRSTFIT_ALLOCATION)
//    {
//        if (n == 0)
//            return 0;
//
//        double min = (n<k)?n:k;
//        return (double)min/n;
//    }
//    else   // Random allocation
//    {
        return (double)k / numBands;
//    }
}

double Binder::computeSinr(unsigned int bgTrafficManagerId, double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus)
{
    double sinr;

    BgTrafficManagerInfo* info = bgTrafficManagerList_.at(bgTrafficManagerId);
    BackgroundTrafficManager* bgTrafficManager = info->bgTrafficManager;

    // TODO configure forceCellLos from config files
    bool ownCellLos = losStatus;
    bool otherLos = losStatus;

    // compute good signal received power
    double recvSignalDbm = bgTrafficManager->getReceivedPower_bgUe(txPower, txPos, rxPos, dir, ownCellLos);

    double totInterference = 0.0;

    // loop through the BackgroundTrafficManagers (one per cell)
    for (unsigned int extId = 0; extId < bgTrafficManagerList_.size(); extId++)
    {
        // skip own interference
        if (extId == bgTrafficManagerId)
            continue;

        BgTrafficManagerInfo* extInfo = bgTrafficManagerList_.at(extId);

        inet::Coord extBsCoord = (extInfo->bgTrafficManager)->getBsCoord();
        double extBsTxPower = (extInfo->bgTrafficManager)->getBsTxPower();

        double overlapPercentage = bgCellsInterferenceMatrix_[bgTrafficManagerId][extId];
        double interferenceDbm = 0.0;
        if (overlapPercentage > 0)
            interferenceDbm = (extInfo->bgTrafficManager)->getReceivedPower_bgUe(extBsTxPower, extBsCoord, rxPos, dir, otherLos);

        // TODO? save the interference into a matrix, so that it can be used to evaluate if convergence has been reached

        totInterference += (dBmToLinear(interferenceDbm) * overlapPercentage);
    }

    double thermalNoise = -104.5;  // dBm TODO take it from the channel model
    double linearNoise = dBmToLinear(thermalNoise);
    double denomDbm = linearToDBm(linearNoise + totInterference);
    sinr = recvSignalDbm - denomDbm;

    return sinr;
}


double Binder::computeRequestedRbsFromSinr(double sinr, double reqLoad)
{
    const double MIN_SINR = -10;                    // TODO fix
    const double MAX_SINR = 30;                     // TODO fix
    const double MAX_SPECTRAL_EFFICIENCY = 1.44 * 1000000;  // TODO fix value

    if( sinr <= MIN_SINR )
        return 0;
    if (sinr > MAX_SINR)
        sinr = MAX_SINR;

    double sinrRange = MAX_SINR - MIN_SINR;

    double normalizedSinr = (sinr + fabs(MIN_SINR)) / sinrRange;

    double bitRate = normalizedSinr * MAX_SPECTRAL_EFFICIENCY;

    //     double rbs = bitRate/reqLoad;
    double rbs = reqLoad / bitRate;

    EV << "Binder::computeRequestedRbsFromSinr - sinr[" << sinr << "] - bitRate[" << bitRate << "] - rbs[" << rbs << "]" << endl;

    return rbs;
}
