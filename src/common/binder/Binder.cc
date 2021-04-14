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
    UplinkTransmissionMap::iterator it = ulTransmissionMap_.begin();
    for (; it != ulTransmissionMap_.end(); ++it)
    {
        double carrierFrequency = it->first;
        int numCarrierBands = componentCarriers_[carrierFrequency].numBands;
        it->second[PREV_TTI] = it->second[CURR_TTI];
        it->second[CURR_TTI].clear();
        it->second[CURR_TTI].resize(numCarrierBands);
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

    if (ulTransmissionMap_.find(carrierFreq) == ulTransmissionMap_.end())
    {
        int numCarrierBands = componentCarriers_[carrierFreq].numBands;
        ulTransmissionMap_[carrierFreq].resize(2);
        ulTransmissionMap_[carrierFreq][PREV_TTI].resize(numCarrierBands);
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
}

const std::vector<UeAllocationInfo>* Binder::getUlTransmissionMap(double carrierFreq, UlTransmissionMapTTI t, Band b)
{
    if (ulTransmissionMap_.find(carrierFreq) == ulTransmissionMap_.end())
        return NULL;
    return &(ulTransmissionMap_[carrierFreq][t][b]);
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


void Binder::registerMulticastGroup(MacNodeId nodeId, int32 groupId)
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

bool Binder::isInMulticastGroup(MacNodeId nodeId, int32 groupId)
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
