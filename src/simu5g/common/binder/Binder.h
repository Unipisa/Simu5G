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

#ifndef _BINDER_H_
#define _BINDER_H_

#include <string>

#include <inet/networklayer/contract/ipv4/Ipv4Address.h>
#include <inet/networklayer/common/L3Address.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/blerCurves/PhyPisaData.h"
#include "simu5g/nodes/ExtCell.h"
#include "simu5g/stack/mac/LteMacBase.h"

namespace simu5g {

using namespace omnetpp;

class UeStatsCollector;


struct NodeInfo {
    opp_component_ptr<cModule> moduleRef;
    opp_component_ptr<LteMacBase> macModule;

    NodeInfo() {}
    NodeInfo(cModule *moduleRef) : moduleRef(moduleRef) {}
};

/**
 * The Binder module has one instance in the whole network.
 * It stores global mapping tables with OMNeT++ module IDs,
 * IP addresses, etc.
 */
class Binder : public cSimpleModule
{
  private:

    // name of the system (top-level) module
    std::string networkName_;

    typedef std::map<MacNodeId, std::map<MacNodeId, bool>> DeployedUesMap;

    std::map<inet::Ipv4Address, MacNodeId> ipAddressToMacNodeId_;
    std::map<inet::Ipv4Address, MacNodeId> ipAddressToNrMacNodeId_;

    // Consolidated node information - replaces nodeIds_, macNodeIdToModuleName_, macNodeIdToModuleRef_, macNodeIdToModule_
    std::map<MacNodeId, NodeInfo> nodeInfoMap_;

    std::vector<MacNodeId> servingNode_;  // ueId -> servingEnbId
    std::vector<MacNodeId> secondaryNodeToMasterNodeOrSelf_;

    // stores the IP address of the MEC hosts in the simulation
    std::set<inet::L3Address> mecHostAddress_;

    // for GTP tunneling with MEC hosts involved
    // this map associates the L3 address of the Virt. Infrastructure of a MEC host to
    // the IP address of the corresponding UPF
    std::map<inet::L3Address, inet::L3Address> mecHostToUpfAddress_;

    // list of static external cells. Used for intercell interference evaluation
    std::map<GHz, ExtCellList> extCellList_;

    // list of static external cells. Used for intercell interference evaluation
    std::map<GHz, BackgroundSchedulerList> bgSchedulerList_;

    // list of all eNBs. Used for inter-cell interference evaluation
    std::vector<EnbInfo *> enbList_;

    // list of all UEs. Used for inter-cell interference evaluation
    std::vector<UeInfo *> ueList_;

    // list of all background traffic managers. Used for background UEs CQI computation
    std::vector<BgTrafficManagerInfo *> bgTrafficManagerList_;

    typedef std::map<unsigned int, std::map<unsigned int, double>> BgInterferenceMatrix;
    // map of maps storing the mutual interference between BG cells
    BgInterferenceMatrix bgCellsInterferenceMatrix_;
    // map of maps storing the mutual interference between BG UEs
    BgInterferenceMatrix bgUesInterferenceMatrix_;
    // maximum data rate achievable in one RB (NED parameter)
    double maxDataRatePerRb_;

    // counters for assigning MacNodeIds
    unsigned int macNodeIdCounterEnb_ = num(ENB_MIN_ID); // eNodeB and gNodeB
    unsigned int macNodeIdCounterUe_ = num(UE_MIN_ID);
    unsigned int macNodeIdCounterNrUe_ = num(NR_UE_MIN_ID);

    DeployedUesMap dMap_; // DeployedUes --> Master Mapping

    /*
     * Carrier Aggregation support
     */
    // total number of logical bands in the system. These bands
    unsigned int totalBands_ = 0;
    CarrierInfoMap componentCarriers_;

    // for each carrier, store the UEs that are able to use it
    typedef std::map<GHz, UeSet> CarrierUeMap;
    CarrierUeMap carrierUeMap_;

    // hack to link carrier freq to numerology index
    std::map<GHz, NumerologyIndex> carrierFreqToNumerologyIndex_;
    // max numerology index used by UEs
    std::vector<NumerologyIndex> ueMaxNumerologyIndex_;
    // set of numerologies used by each UE
    std::map<MacNodeId, std::set<NumerologyIndex>> ueNumerologyIndex_;

    /*
     * Uplink interference support
     */
    typedef std::map<GHz, std::vector<std::vector<std::vector<UeAllocationInfo>>>> UplinkTransmissionMap;
    // for each carrier frequency, for both previous and current TTIs, for each RB, stores the UE (nodeId and ref to the PHY module) that transmitted/are transmitting within that RB
    UplinkTransmissionMap ulTransmissionMap_;
    // TTI of the last update of the UL band status
    simtime_t lastUpdateUplinkTransmissionInfo_;
    // TTI of the last UL transmission (used for optimization purposes, see initAndResetUlTransmissionInfo() )
    simtime_t lastUplinkTransmission_;

    /*
     * X2 Support
     */
    typedef std::map<X2NodeId, std::list<int>> X2ListeningPortMap;
    X2ListeningPortMap x2ListeningPorts_;

    std::map<MacNodeId, std::map<MacNodeId, inet::L3Address>> x2PeerAddress_;

    /*
     * D2D Support
     */
    // determines if two D2D-capable UEs are communicating in D2D mode or Infrastructure Mode
    std::map<MacNodeId, std::map<MacNodeId, LteD2DMode>> d2dPeeringMap_;

    /*
     * Multicast support
     */
    // register here the IDs of the multicast group where UEs participate
    typedef std::set<uint32_t> MulticastGroupIdSet;
    std::map<MacNodeId, MulticastGroupIdSet> nodeGroupMemberships_;
    std::set<MacNodeId> multicastTransmitterSet_;

    /*
     * Handover support
     */
    // store the id of the UEs that are performing handover
    std::set<MacNodeId> ueHandoverTriggered_;
    std::map<MacNodeId, std::pair<MacNodeId, MacNodeId>> handoverTriggered_;

  protected:
    void initialize(int stages) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override {}

    void finish() override;

    // helpers
    bool isValidNodeId(MacNodeId  nodeId) const;
    LteD2DMode computeD2DCapability(MacNodeId src, MacNodeId dst);

  public:
    Binder() {}

    unsigned int getTotalBands()
    {
        return totalBands_;
    }

    ~Binder() override
    {
        for (auto enb : enbList_)
            delete enb;

        for (auto bg : bgTrafficManagerList_)
            delete bg;

        for (auto ue : ueList_)
            delete ue;
    }

    std::string& getNetworkName()
    {
        return networkName_;
    }

    /**
     * Registers a carrier to the global Binder module
     */
    void registerCarrier(GHz carrierFrequency, unsigned int carrierNumBands, unsigned int numerologyIndex,
            bool useTdd = false, unsigned int tddNumSymbolsDl = 0, unsigned int tddNumSymbolsUl = 0);

    /**
     * Registers a UE to a given carrier
     */
    void registerCarrierUe(GHz carrierFrequency, unsigned int numerologyIndex, MacNodeId ueId);

    /**
     * Returns the set of UEs enabled on the given carrier
     */
    const UeSet& getCarrierUeSet(GHz carrierFrequency);

    /**
     * Returns the max numerology index used by the given UE
     */
    NumerologyIndex getUeMaxNumerologyIndex(MacNodeId ueId);

    /**
     * Returns the numerology indices used by the given UE
     */
    const std::set<NumerologyIndex> *getUeNumerologyIndex(MacNodeId ueId);

    /**
     * Returns the numerology associated to a carrier frequency
     */
    NumerologyIndex getNumerologyIndexFromCarrierFreq(GHz carrierFreq) { return carrierFreqToNumerologyIndex_[carrierFreq]; }

    /**
     * Returns the slot duration associated to the numerology (in seconds)
     */
    double getSlotDurationFromNumerologyIndex(NumerologyIndex numerologyIndex) { return pow(2.0, (-1) * (int)numerologyIndex) / 1000.0; }

    /**
     * Compute slot format object given the number of DL and UL symbols
     */
    SlotFormat computeSlotFormat(bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl);

    /**
     * Returns the slot format for the given carrier
     */
    SlotFormat getSlotFormat(GHz carrierFrequency);

    /**
     * Registers a node to the global Binder module. The return value is the
     * Binder-assigned unique MacNodeId of the node. The masterId argument
     * has dual purpose depending on the type: for UE, it specifies the
     * serving nodeB id; for an eNB/gNB, it specifies the master nodeB
     * for this node, provided it is a secondary node in a Dual-Connectivity setup.
     * isNR specifies whether an LTE or 5G NR nodeId of a UE is to be assigned.
     */
    MacNodeId registerNode(cModule *nodeModule, RanNodeType type, MacNodeId masterId = NODEID_NONE, bool isNr = false);

    /**
     * Un-registers a node from the global Binder module.
     */
    void unregisterNode(MacNodeId id);

    /**
     * Binds an UE with its serving eNodeB/gNodeB. Invoked at the start of
     * the simulation and on handovers.
     */
    void registerServingNode(MacNodeId enbId, MacNodeId ueId);

    /**
     * Unregisters the UE from its current eNodeB/gNodeB. Invoked e.g. on handovers.
     */
    void unregisterServingNode(MacNodeId enbId, MacNodeId ueId);

    /**
     * Returns the serving eNodeB/gNodeB of an UE, or NODEID_NONE if there is none.
     */
    MacNodeId getServingNode(MacNodeId ueId);

    /**
     * registerMasterNode()
     *
     * It registers the secondary node to its master node (used for dual connectivity)
     *
     * @param masterId MacNodeId of the Master
     * @param slaveId MacNodeId of the Slave
     */
    void registerMasterNode(MacNodeId masterId, MacNodeId slaveId);

    /**
     * Returns true if the node exists.
     */
    bool nodeExists(MacNodeId nodeId) { return nodeInfoMap_.find(nodeId) != nodeInfoMap_.end(); }

    /**
     * Returns nullptr if not found.
     */
    cModule *getNodeModule(MacNodeId nodeId);

    /*
     * getNodeInfoMap returns information on all nodes in a map
     */
    const std::map<MacNodeId, NodeInfo>& getNodeInfoMap() const { return nodeInfoMap_; }

    /*
     * getMacFromMacNodeId() returns the reference to the LteMacBase module
     * given the MacNodeId of a node
     *
     * @param id MacNodeId of the module
     * @return LteMacBase* of the module
     */
    LteMacBase *getMacFromMacNodeId(MacNodeId id);

    /**
     * For an UE, returns the serving eNodeB/gNodeB; for an eNodeB/gNodeB, returns the nodeId (the arg).
     */
    MacNodeId getNextHop(MacNodeId nodeId);

    /**
     * In a Dual Connectivity / Split Bearer setup, returns the Master Node (MeNB, MN)
     * for the given Secondary Node (SeNB, SN).
     */
    //TODO add  MacNodeId ueId arg: In LTE DC, the roles (master/secondary) are per UE, not per bearer.
    MacNodeId getMasterNodeOrSelf(MacNodeId secondaryEnbId);

    /**
     * TODO add "forUeId" argument, to getMasterNode() too
     */
    MacNodeId getSecondaryNode(MacNodeId masterEnbId);

    /**
     * Returns the MacNodeId for the given IP address
     *
     * @param address IP address
     * @return MacNodeId corresponding to the IP address
     */
    MacNodeId getMacNodeId(inet::Ipv4Address address)
    {
        if (ipAddressToMacNodeId_.find(address) == ipAddressToMacNodeId_.end())
            return NODEID_NONE;
        MacNodeId nodeId = ipAddressToMacNodeId_[address];

        // if the UE is disconnected (its master node is 0), check the NR node Id
        if (getNextHop(nodeId) == NODEID_NONE)
            return getNrMacNodeId(address);
        return nodeId;
    }

    /**
     * Returns the MacNodeId for the given IP address
     *
     * @param address IP address
     * @return MacNodeId corresponding to the IP address
     */
    MacNodeId getNrMacNodeId(inet::Ipv4Address address)
    {
        if (ipAddressToNrMacNodeId_.find(address) == ipAddressToNrMacNodeId_.end())
            return NODEID_NONE;
        return ipAddressToNrMacNodeId_[address];
    }

    /**
     * author Alessandro Noferi
     *
     * Returns the IP address for the given MacNodeId
     *
     * @param MacNodeId of the node
     * @return IP address corresponding to the MacNodeId
     *
     */
    inet::Ipv4Address getIPv4Address(MacNodeId nodeId)
    {
        for (const auto& kv : ipAddressToMacNodeId_) {
            if (kv.second == nodeId)
                return kv.first;
        }
        for (const auto& kv : ipAddressToNrMacNodeId_) {
            if (kv.second == nodeId)
                return kv.first;
        }
        return inet::Ipv4Address::UNSPECIFIED_ADDRESS;
    }

    /**
     * Returns the X2NodeId for the given IP address
     *
     * @param address IP address
     * @return X2NodeId corresponding to the IP address
     */
    X2NodeId getX2NodeId(inet::Ipv4Address address)
    {
        return getMacNodeId(address);
    }

    /**
     * Associates the given IP address with the given MacNodeId.
     *
     * @param address IP address
     */
    void setMacNodeId(inet::Ipv4Address address, MacNodeId nodeId)
    {
        if (isNrUe(nodeId))
            ipAddressToNrMacNodeId_[address] = nodeId;
        else
            ipAddressToMacNodeId_[address] = nodeId;
    }

    /**
     * Associates the given IP address with the given X2NodeId.
     *
     * @param address IP address
     */
    void setX2NodeId(inet::Ipv4Address address, X2NodeId nodeId)
    {
        setMacNodeId(address, nodeId);
    }

    inet::L3Address getX2PeerAddress(X2NodeId srcId, X2NodeId destId)
    {
        return x2PeerAddress_[srcId][destId];
    }

    void setX2PeerAddress(X2NodeId srcId, X2NodeId destId, inet::L3Address interfAddr)
    {
        x2PeerAddress_[srcId].insert({destId, interfAddr});
    }

    /**
     * Register the address of MEC Hosts in the simulation
     */
    void registerMecHost(const inet::L3Address& mecHostAddress);
    /**
     * Associates the given MEC Host address to its corresponding UPF
     */
    void registerMecHostUpfAddress(const inet::L3Address& mecHostAddress, const inet::L3Address& gtpAddress);
    /**
     * Returns true if the given address belongs to a MEC host
     */
    bool isMecHost(const inet::L3Address& mecHostAddress);
    /**
     * Returns the UPF address corresponding to the given MEC Host address
     */
    const inet::L3Address& getUpfFromMecHost(const inet::L3Address& mecHostAddress);
    /**
     * Associates the given MAC node ID to the module
     */
    void registerModule(MacNodeId nodeId, cModule *module);
    /**
     * Returns the module for the given MAC node ID
     */
    cModule *getModuleByMacNodeId(MacNodeId nodeId);

    /*
     * getDeployedUes() returns the affiliates
     * of a given eNodeB
     */
    ConnectedUesMap getDeployedUes(MacNodeId localId, Direction dir);
    PhyPisaData phyPisaData;

    int getNodeCount() {
        return nodeInfoMap_.size();
    }

    int addExtCell(ExtCell *extCell, GHz carrierFrequency)
    {
        if (extCellList_.find(carrierFrequency) == extCellList_.end())
            extCellList_[carrierFrequency] = ExtCellList();

        extCellList_[carrierFrequency].push_back(extCell);
        return extCellList_[carrierFrequency].size() - 1;
    }

    ExtCellList getExtCellList(GHz carrierFrequency)
    {
        return extCellList_[carrierFrequency];
    }

    int addBackgroundScheduler(BackgroundScheduler *bgScheduler, GHz carrierFrequency)
    {
        if (bgSchedulerList_.find(carrierFrequency) == bgSchedulerList_.end())
            bgSchedulerList_[carrierFrequency] = BackgroundSchedulerList();

        bgSchedulerList_[carrierFrequency].push_back(bgScheduler);
        return bgSchedulerList_[carrierFrequency].size() - 1;
    }

    const BackgroundSchedulerList& getBackgroundSchedulerList(GHz carrierFrequency)
    {
        return bgSchedulerList_[carrierFrequency];
    }

    void addEnbInfo(EnbInfo *info)
    {
        enbList_.push_back(info);
    }

    const std::vector<EnbInfo *>& getEnbList()
    {
        return enbList_;
    }

    void addUeInfo(UeInfo *info)
    {
        ueList_.push_back(info);
    }

    const std::vector<UeInfo *>& getUeList()
    {
        return ueList_;
    }

    void addBgTrafficManagerInfo(BgTrafficManagerInfo *info)
    {
        bgTrafficManagerList_.push_back(info);
    }

    const std::vector<BgTrafficManagerInfo *>& getBgTrafficManagerList()
    {
        return bgTrafficManagerList_;
    }

    Cqi meanCqi(std::vector<Cqi> bandCqi, MacNodeId id, Direction dir);

    Cqi medianCqi(std::vector<Cqi> bandCqi, MacNodeId id, Direction dir);

    /*
     * Uplink interference support
     */
    simtime_t getLastUpdateUlTransmissionInfo();
    void initAndResetUlTransmissionInfo();
    void storeUlTransmissionMap(GHz carrierFreq, Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, LtePhyBase *phy, Direction dir);
    void storeUlTransmissionMap(GHz carrierFreq, Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, TrafficGeneratorBase *trafficGen, Direction dir);  // overloaded function for bgUes
    const std::vector<std::vector<UeAllocationInfo>> *getUlTransmissionMap(GHz carrierFreq, UlTransmissionMapTTI t);
    /*
     * X2 Support
     */
    void registerX2Port(X2NodeId nodeId, int port);
    int getX2Port(X2NodeId nodeId);

    /*
     * D2D Support
     */
    bool checkD2DCapability(MacNodeId src, MacNodeId dst);
    bool getD2DCapability(MacNodeId src, MacNodeId dst);

    std::map<MacNodeId, std::map<MacNodeId, LteD2DMode>> *getD2DPeeringMap();
    void setD2DMode(MacNodeId src, MacNodeId dst, LteD2DMode mode);
    LteD2DMode getD2DMode(MacNodeId src, MacNodeId dst);
    bool isFrequencyReuseEnabled(MacNodeId nodeId);
    /*
     * Multicast Support
     */
    // add the group to the set of multicast group of nodeId
    void joinMulticastGroup(MacNodeId nodeId, int32_t groupId);
    // check if the node is enrolled in the group
    bool isInMulticastGroup(MacNodeId nodeId, int32_t groupId);
    // add one multicast transmitter
    void addD2DMulticastTransmitter(MacNodeId nodeId);
    // get multicast transmitters
    std::set<MacNodeId>& getD2DMulticastTransmitters();

    /*
     *  Handover support
     */
    void addUeHandoverTriggered(MacNodeId nodeId);
    bool hasUeHandoverTriggered(MacNodeId nodeId);
    void removeUeHandoverTriggered(MacNodeId nodeId);

    void addHandoverTriggered(MacNodeId nodeId, MacNodeId srcId, MacNodeId destId);
    const std::pair<MacNodeId, MacNodeId> *getHandoverTriggered(MacNodeId nodeId);
    void removeHandoverTriggered(MacNodeId nodeId);

    void updateUeInfoCellId(MacNodeId nodeId, MacCellId cellId);

    /*
     *  Background UEs and cells Support
     */
    void computeAverageCqiForBackgroundUes();
    void updateMutualInterference(unsigned int bgTrafficManagerId, unsigned int numBands, Direction dir);
    double computeInterferencePercentageDl(double n, double k, unsigned int numBands);
    double computeInterferencePercentageUl(double n, double k, double nTotal, double kTotal);
    double computeSinr(unsigned int bgTrafficManagerId, int bgUeId, double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus);
    double computeRequestedRbsFromSinr(double sinr, double reqLoad);

    /*
     * author Alessandro Noferi.
     *
     * UeStatsCollector management
     */

    /* this method adds the UeStatsCollector reference to the baseStationStatsCollector
     * structure.
     * @params:
     *  ue: MacNodeId of the ue
     *  ueCollector: reference to the collector
     *  cell: MacCellId of the target eNB
     */
    void addUeCollectorToEnodeB(MacNodeId ue, UeStatsCollector *ueCollector, MacCellId cell);

    /* this method moves the UeStatsCollector reference between the eNB/gNB's baseStationStatsCollector
     * structure.
     * @params:
     *  ue: MacNodeId of the ue
     *  oldCell: MacCellId of the source eNB
     *  newCell: MacCellId of the target eNB
     */
    void moveUeCollector(MacNodeId ue, MacCellId oldCell, MacCellId newCell);

    RanNodeType getBaseStationTypeById(MacNodeId);

    // Moved from LteCommon - getter functions that were taking Binder as first parameter
    CellInfo *getCellInfoByNodeId(MacNodeId nodeId);
    cModule *getPhyByNodeId(MacNodeId nodeId);
    cModule *getMacByNodeId(MacNodeId nodeId);
    cModule *getRlcByNodeId(MacNodeId nodeId, LteRlcType rlcType);
    cModule *getPdcpByNodeId(MacNodeId nodeId);

};

} //namespace

#endif
