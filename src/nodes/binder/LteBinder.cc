//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "nodes/binder/LteBinder.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include <cctype>
#include <algorithm>

#include "corenetwork/lteCellInfo/LteCellInfo.h"
#include "nodes/InternetMux.h"
#include "corenetwork/statsCollector/EnodeBStatsCollector.h"
#include "corenetwork/statsCollector/UeStatsCollector.h"

using namespace std;
using namespace inet;

Define_Module(LteBinder);

void LteBinder::registerCarrier(double carrierFrequency, unsigned int carrierNumBands, unsigned int numerologyIndex, bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl)
{
    CarrierInfoMap::iterator it = componentCarriers_.find(carrierFrequency);
    if (it != componentCarriers_.end() && carrierNumBands <= componentCarriers_[carrierFrequency].numBands)
    {
        EV << "LteBinder::registerCarrier - Carrier @ " << carrierFrequency << "GHz already registered" << endl;
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

        EV << "LteBinder::registerCarrier - Registered component carrier @ " << carrierFrequency << "GHz" << endl;

        carrierFreqToNumerologyIndex_[carrierFrequency] = numerologyIndex;

        // add new (empty) entry to carrierUeMap
        std::set<MacNodeId> tempSet;
        carrierUeMap_[carrierFrequency] = tempSet;
    }
}

void LteBinder::registerCarrierUe(double carrierFrequency, unsigned int numerologyIndex, MacNodeId ueId)
{
    // check if carrier exists in the system
    CarrierUeMap::iterator it = carrierUeMap_.find(carrierFrequency);
    if (it == carrierUeMap_.end())
        throw cRuntimeError("LteBinder::registerCarrierUe - Carrier [%fGHz] not found", carrierFrequency);

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

const UeSet& LteBinder::getCarrierUeSet(double carrierFrequency)
{
    CarrierUeMap::iterator it = carrierUeMap_.find(carrierFrequency);
    if (it == carrierUeMap_.end())
        throw cRuntimeError("LteBinder::getCarrierUeSet - Carrier [%fGHz] not found", carrierFrequency);

    return carrierUeMap_[carrierFrequency];
}

NumerologyIndex LteBinder::getUeMaxNumerologyIndex(MacNodeId ueId)
{
    return ueMaxNumerologyIndex_[ueId];
}

const std::set<NumerologyIndex>* LteBinder::getUeNumerologyIndex(MacNodeId ueId)
{
    if (ueNumerologyIndex_.find(ueId) == ueNumerologyIndex_.end())
        return NULL;
    return &ueNumerologyIndex_[ueId];
}

SlotFormat LteBinder::computeSlotFormat(bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl)
{
    SlotFormat sf;
    if (!useTdd)
    {
        sf.tdd = false;
    }
    else
    {
        unsigned int rbxDl = 7;  // TODO replace with the parameter obtained from NED file once you moved the function to the Component Carrier

        sf.tdd = true;
        unsigned int numSymbols = rbxDl * 2;

        if (tddNumSymbolsDl+tddNumSymbolsUl > numSymbols)
            throw cRuntimeError("LteBinder::computeSlotFormat - Number of symbols not valid - DL[%d] UL[%d]", tddNumSymbolsDl,tddNumSymbolsUl);

        sf.numDlSymbols = tddNumSymbolsDl;
        sf.numUlSymbols = tddNumSymbolsUl;
        sf.numFlexSymbols = numSymbols - tddNumSymbolsDl - tddNumSymbolsUl;
    }
    return sf;
}

SlotFormat LteBinder::getSlotFormat(double carrierFrequency)
{
    CarrierInfoMap::iterator it = componentCarriers_.find(carrierFrequency);
    if (it == componentCarriers_.end())
        throw cRuntimeError("LteBinder::getSlotFormat - Carrier [%fGHz] not found", carrierFrequency);

    return it->second.slotFormat;
}


void LteBinder::unregisterNode(MacNodeId id)
{
    EV << NOW << " LteBinder::unregisterNode - unregistering node " << id << endl;

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

MacNodeId LteBinder::registerNode(cModule *module, LteNodeType type, MacNodeId masterId, bool registerNr)
{
    Enter_Method_Silent("registerNode");

    MacNodeId macNodeId = -1;

    if (type == UE)
    {
        if (!registerNr)
            macNodeId = macNodeIdCounter_[2]++;
        else
            macNodeId = macNodeIdCounter_[3]++;
    }
    else if (type == RELAY)
    {
        macNodeId = macNodeIdCounter_[1]++;
    }
    else if (type == ENODEB || type == GNODEB)
    {
        macNodeId = macNodeIdCounter_[0]++;
    }

    EV << "LteBinder : Assigning to module " << module->getName()
       << " with OmnetId: " << module->getId() << " and MacNodeId " << macNodeId
       << "\n";

    // registering new node to LteBinder

    nodeIds_[macNodeId] = module->getId();

    if (!registerNr)
        module->par("macNodeId") = macNodeId;
    else
        module->par("nrMacNodeId") = macNodeId;

    if (type == RELAY || type == UE)
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

void LteBinder::registerNextHop(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method_Silent("registerNextHop");

    EV << "LteBinder : Registering slave " << slaveId << " to master "
       << masterId << "\n";

    if (masterId != slaveId)
    {
        dMap_[masterId][slaveId] = true;
    }

    if (nextHop_.size() <= slaveId)
        nextHop_.resize(slaveId + 1);
    nextHop_[slaveId] = masterId;
}

void LteBinder::registerMasterNode(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method_Silent("registerMasterNode");
    EV << "LteBinder : Registering slave " << slaveId << " to master "
       << masterId << "\n";

    if (secondaryNodeToMasterNode_.size() <= slaveId)
        secondaryNodeToMasterNode_.resize(slaveId + 1);

    if (masterId == 0)  // this node is a master itself
        masterId = slaveId;
    secondaryNodeToMasterNode_[slaveId] = masterId;
}

void LteBinder::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
        phyPisaData.setBlerShift(par("blerShift"));
}

void LteBinder::unregisterNextHop(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method_Silent("unregisterNextHop");
    EV << "LteBinder : Unregistering slave " << slaveId << " from master "
       << masterId << "\n";
    dMap_[masterId][slaveId] = false;

    if (nextHop_.size() <= slaveId)
        return;
    nextHop_[slaveId] = 0;
}

OmnetId LteBinder::getOmnetId(MacNodeId nodeId)
{
    std::map<int, OmnetId>::iterator it = nodeIds_.find(nodeId);
    if(it != nodeIds_.end())
        return it->second;
    return 0;
}

std::map<int, OmnetId>::const_iterator LteBinder::getNodeIdListBegin()
{
    return nodeIds_.begin();
}

std::map<int, OmnetId>::const_iterator LteBinder::getNodeIdListEnd()
{
    return nodeIds_.end();
}

MacNodeId LteBinder::getMacNodeIdFromOmnetId(OmnetId id){
	std::map<int, OmnetId>::iterator it;
	for (it = nodeIds_.begin(); it != nodeIds_.end(); ++it )
	    if (it->second == id)
	        return it->first;
	return 0;
}

LteMacBase* LteBinder::getMacFromMacNodeId(MacNodeId id)
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

MacNodeId LteBinder::getNextHop(MacNodeId slaveId)
{
    Enter_Method_Silent("getNextHop");
    if (slaveId >= nextHop_.size())
        throw cRuntimeError("LteBinder::getNextHop(): bad slave id %d", slaveId);
    return nextHop_[slaveId];
}

MacNodeId LteBinder::getMasterNode(MacNodeId slaveId)
{
    Enter_Method_Silent("getMasterNode");
    if (slaveId >= secondaryNodeToMasterNode_.size())
        throw cRuntimeError("LteBinder::getMasterNode(): bad slave id %d", slaveId);
    return secondaryNodeToMasterNode_[slaveId];
}

void LteBinder::registerName(MacNodeId nodeId, const char* moduleName)
{
    int len = strlen(moduleName);
    macNodeIdToModuleName_[nodeId] = new char[len+1];
    strcpy(macNodeIdToModuleName_[nodeId], moduleName);
}

const char* LteBinder::getModuleNameByMacNodeId(MacNodeId nodeId)
{
    if (macNodeIdToModuleName_.find(nodeId) == macNodeIdToModuleName_.end())
        throw cRuntimeError("LteBinder::getModuleNameByMacNodeId - node ID not found");
    return macNodeIdToModuleName_[nodeId];
}

ConnectedUesMap LteBinder::getDeployedUes(MacNodeId localId, Direction dir)
{
    Enter_Method_Silent("getDeployedUes");
    return dMap_[localId];
}

simtime_t LteBinder::getLastUpdateUlTransmissionInfo()
{
    return lastUpdateUplinkTransmissionInfo_;
}

void LteBinder::initAndResetUlTransmissionInfo()
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
        double carrierFrequency = it->first;
        int numCarrierBands = componentCarriers_[carrierFrequency].numBands;

        // the second element (i.e. referring to the old time slot) becomes the first element
        it->second.erase(it->second.begin());
        // push a new element (i.e. referring to the current time slot)
        it->second.push_back(std::vector<std::vector<UeAllocationInfo> >());
        it->second[CURR_TTI].resize(numCarrierBands);
    }
    lastUpdateUplinkTransmissionInfo_ = NOW;
}

void LteBinder::storeUlTransmissionMap(double carrierFreq, Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, LtePhyBase* phy, Direction dir)
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

    lastUplinkTransmission_ = NOW;
}
const std::vector<std::vector<UeAllocationInfo> >* LteBinder::getUlTransmissionMap(double carrierFreq, UlTransmissionMapTTI t)
{
    if (ulTransmissionMap_.find(carrierFreq) == ulTransmissionMap_.end())
        return NULL;
    return &(ulTransmissionMap_[carrierFreq][t]);
}


void LteBinder::registerX2Port(X2NodeId nodeId, int port)
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

int LteBinder::getX2Port(X2NodeId nodeId)
{
    if (x2ListeningPorts_.find(nodeId) == x2ListeningPorts_.end() )
        throw cRuntimeError("LteBinder::getX2Port - No ports available on node %d", nodeId);

    int port = x2ListeningPorts_[nodeId].front();
    x2ListeningPorts_[nodeId].pop_front();
    return port;
}

Cqi LteBinder::meanCqi(std::vector<Cqi> bandCqi,MacNodeId id,Direction dir)
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

Cqi LteBinder::medianCqi(std::vector<Cqi> bandCqi,MacNodeId id,Direction dir)
{
    std::sort(bandCqi.begin(),bandCqi.end());

    int medianPoint = bandCqi.size()/2;

    EV << "LteBinder::medianCqi - median point is " << bandCqi.size() << "/2 = " << medianPoint << ". MedianCqi = " << bandCqi[medianPoint] << endl;

    return bandCqi[medianPoint];

}

bool LteBinder::checkD2DCapability(MacNodeId src, MacNodeId dst)
{
    if (src < UE_MIN_ID || (src >= macNodeIdCounter_[2] && src < NR_UE_MIN_ID) || src >= macNodeIdCounter_[3]
            || dst < UE_MIN_ID || (dst >= macNodeIdCounter_[2] && dst < NR_UE_MIN_ID) || dst >= macNodeIdCounter_[3])
        throw cRuntimeError("LteBinder::checkD2DCapability - Node Id not valid. Src %d Dst %d", src, dst);

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

            EV << "LteBinder::checkD2DCapability - UE " << src << " may transmit to UE " << dst << " using D2D (current mode " << ((d2dPeeringMap_[src][dst] == DM) ? "DM)" : "IM)") << endl;

            // this is a D2D-capable flow
            return true;
        }
        else
        {
            EV << "LteBinder::checkD2DCapability - UE " << src << " may not transmit to UE " << dst << " using D2D (UE " << dst << " is not D2D capable)" << endl;
            // this is not a D2D-capable flow
            return false;
        }

    }

    // an entry is present, hence this is a D2D-capable flow
    return true;
}

bool LteBinder::getD2DCapability(MacNodeId src, MacNodeId dst)
{
    if (src < UE_MIN_ID || (src >= macNodeIdCounter_[2] && src < NR_UE_MIN_ID) || src >= macNodeIdCounter_[3]
            || dst < UE_MIN_ID || (dst >= macNodeIdCounter_[2] && dst < NR_UE_MIN_ID) || dst >= macNodeIdCounter_[3])
        throw cRuntimeError("LteBinder::getD2DCapability - Node Id not valid. Src %d Dst %d", src, dst);

    // if the entry is missing, returns false
    if (d2dPeeringMap_.find(src) == d2dPeeringMap_.end() || d2dPeeringMap_[src].find(dst) == d2dPeeringMap_[src].end())
        return false;

    // the entry exists, no matter if it is DM or IM
    return true;
}

std::map<MacNodeId, std::map<MacNodeId, LteD2DMode> >* LteBinder::getD2DPeeringMap()
{
    return &d2dPeeringMap_;
}

LteD2DMode LteBinder::getD2DMode(MacNodeId src, MacNodeId dst)
{
    if (src < UE_MIN_ID || (src >= macNodeIdCounter_[2] && src < NR_UE_MIN_ID) || src >= macNodeIdCounter_[3]
            || dst < UE_MIN_ID || (dst >= macNodeIdCounter_[2] && dst < NR_UE_MIN_ID) || dst >= macNodeIdCounter_[3])
        throw cRuntimeError("LteBinder::getD2DMode - Node Id not valid. Src %d Dst %d", src, dst);

    return d2dPeeringMap_[src][dst];
}

bool LteBinder::isFrequencyReuseEnabled(MacNodeId nodeId)
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


void LteBinder::registerMulticastGroup(MacNodeId nodeId, int32_t groupId)
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

bool LteBinder::isInMulticastGroup(MacNodeId nodeId, int32_t groupId)
{
    if (multicastGroupMap_.find(nodeId) == multicastGroupMap_.end())
        return false;   // the node is not enrolled in any group
    if (multicastGroupMap_[nodeId].find(groupId) == multicastGroupMap_[nodeId].end())
        return false;   // the node is not enrolled in the given group

    return true;
}

void LteBinder::addD2DMulticastTransmitter(MacNodeId nodeId)
{
    multicastTransmitterSet_.insert(nodeId);
}

std::set<MacNodeId>& LteBinder::getD2DMulticastTransmitters()
{
    return multicastTransmitterSet_;
}


void LteBinder::updateUeInfoCellId(MacNodeId id, MacCellId newCellId)
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

void LteBinder::addUeHandoverTriggered(MacNodeId nodeId)
{
    ueHandoverTriggered_.insert(nodeId);
}

bool LteBinder::hasUeHandoverTriggered(MacNodeId nodeId)
{
    if (ueHandoverTriggered_.find(nodeId) == ueHandoverTriggered_.end())
        return false;
    return true;
}

void LteBinder::removeUeHandoverTriggered(MacNodeId nodeId)
{
    ueHandoverTriggered_.erase(nodeId);
}

void LteBinder::addHandoverTriggered(MacNodeId nodeId, MacNodeId srcId, MacNodeId destId)
{
    std::pair<MacNodeId, MacNodeId> p(srcId, destId);
    handoverTriggered_[nodeId] = p;
}

const std::pair<MacNodeId, MacNodeId>* LteBinder::getHandoverTriggered(MacNodeId nodeId)
{
    if (handoverTriggered_.find(nodeId) == handoverTriggered_.end())
        return NULL;
    return &handoverTriggered_[nodeId];
}

void LteBinder::removeHandoverTriggered(MacNodeId nodeId)
{
    std::map<MacNodeId, std::pair<MacNodeId, MacNodeId> >::iterator it = handoverTriggered_.find(nodeId);
    if (it!=handoverTriggered_.end())
        handoverTriggered_.erase(it);
}


/*
 * @author Alessandro Noferi
 */

void LteBinder::addUeCollectorToEnodeB(MacNodeId ue, UeStatsCollector* ueCollector , MacNodeId cell)
{
    EV << "LteBinder::addUeCollector"<< endl;
    std::vector<EnbInfo*>::iterator it = enbList_.begin(), end = enbList_.end();
    cModule *enb = nullptr;
    EnodeBStatsCollector * enbColl = nullptr;

    // check if the collector is already present in a cell
    for(; it != end ; ++it)
    {
        enb = (*it)->eNodeB;
        if (enb->getSubmodule("collector") != nullptr)
        {
            enbColl = check_and_cast<EnodeBStatsCollector *>(enb->getSubmodule("collector"));
            if(enbColl->hasUeCollector(ue))
            {
                EV << "LteBinder::addUeCollector - UeCollector for node [" << ue << "] already present in eNodeB [" << (*it)->id << "]" << endl;
                throw cRuntimeError("LteBinder::addUeCollector - UeCollector for node [%d] already present in eNodeB [%d]", ue,(*it)->id ) ;
            }
        }
        else
        {
            EV << "LteBinder::addUeCollector - eNodeB [" << (*it)->id << "] does not have the eNodeBStatsCollector" << endl;
//            throw cRuntimeError("LteBinder::addUeCollector - eNodeB [%d] does not have the eNodeBStatsCollector", (*it)->id ) ;

        }

    }

    // no cell has the UeCollector, add it
    enb = getParentModule()->getSubmodule(getModuleNameByMacNodeId(cell));
    if (enb->getSubmodule("collector") != nullptr)
    {
        enbColl = check_and_cast<EnodeBStatsCollector *>(enb->getSubmodule("collector"));
        enbColl->addUeCollector(ue, ueCollector);
        EV << "LteBinder::addUeCollector - UeCollector for node [" << ue << "] added to eNodeB [" << cell << "]" << endl;
    }
    else
    {
        EV << "LteBinder::addUeCollector - eNodeB [" << cell << "] does not have the eNodeBStatsCollector." <<
              " UeCollector for node [" << ue << "] NOT added to eNodeB [" << cell << "]" << endl;
//        throw cRuntimeError("LteBinder::addUeCollector - eNodeBStatsCollector not present in eNodeB [%d]",(*it)->id ) ;
    }
}


void LteBinder::moveUeCollector(MacNodeId ue, MacCellId oldCell, MacCellId newCell)
{
    EV << "LteBinder::moveUeCollector" << endl;
    LteNodeType oldCellType = getBaseStationTypeById(oldCell);
    LteNodeType newCellType = getBaseStationTypeById(newCell);

    // get and remove the UeCollector from the OldCell
    const char* cellModuleName = getModuleNameByMacNodeId(oldCell); // eNodeB module name
    cModule *oldEnb = getParentModule()->getSubmodule(cellModuleName); //  eNobe module
    EnodeBStatsCollector * enbColl = nullptr;
    UeStatsCollector * ueColl = nullptr;
    if (oldEnb->getSubmodule("collector") != nullptr)
    {
       enbColl = check_and_cast<EnodeBStatsCollector *>(oldEnb->getSubmodule("collector"));
       if(enbColl->hasUeCollector(ue))
       {
           ueColl = enbColl->getUeCollector(ue);
           ueColl->resetStats();
           enbColl->removeUeCollector(ue);
       }
       else
       {
           throw cRuntimeError("LteBinder::moveUeCollector - UeStatsCollector of node [%d] not present in eNodeB [%d]", ue,oldCell ) ;
       }
    }
    else
    {
        throw cRuntimeError("LteBinder::moveUeCollector - eNodeBStatsCollector not present in eNodeB [%d]", oldCell) ;
    }
    // if the two base station are the same type, just move the collector
    if(oldCellType == newCellType)
    {
        addUeCollectorToEnodeB(ue, ueColl, newCell);
    }
    else
    {
        if(newCellType == GNODEB)
        {
            // retrieve NrUeCollector
            cModule *ueModule = getModuleByPath(getModuleNameByMacNodeId(ue));
            if(ueModule->findSubmodule("NRueCollector") == -1)
                ueColl = check_and_cast<UeStatsCollector *>(ueModule->getSubmodule("NRueCollector"));
            else
                throw cRuntimeError("LteBinder::moveUeCollector - Ue [%d] has not got NRueCollector required for the gNB", ue) ;
            addUeCollectorToEnodeB(ue, ueColl, newCell);

        }
        else if (newCellType == ENODEB)
        {
            // retrieve NrUeCollector
            cModule *ueModule = getModuleByPath(getModuleNameByMacNodeId(ue));
            if(ueModule->findSubmodule("ueCollector") == -1)
                ueColl = check_and_cast<UeStatsCollector *>(ueModule->getSubmodule("ueCollector"));
            else
                throw cRuntimeError("LteBinder::moveUeCollector - Ue [%d] has not got ueCollector required for the eNB", ue) ;
            addUeCollectorToEnodeB(ue, ueColl, newCell);
        }
        else
        {
            throw cRuntimeError("LteBinder::moveUeCollector - The new cell is not a cell [%d]", newCellType) ;
        }
    }
}

LteNodeType LteBinder::getBaseStationTypeById(MacNodeId cellId)
{
    cModule *module = getModuleByPath(getModuleNameByMacNodeId(cellId));
    std::string nodeType;
    if(module->hasPar("nodeType"))
    {
        nodeType = module->par("nodeType").stdstringValue();
        if(nodeType.compare("ENODEB") == 0)
            return ENODEB;
        else if(nodeType.compare("GNODEB") == 0)
            return GNODEB;
        else
            return UNKNOWN_NODE_TYPE;
    }
    else
    {
        return UNKNOWN_NODE_TYPE;
    }
}



