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

// Forward declaration
class FlowDescriptor;

namespace simu5g {

using namespace omnetpp;

class UeStatsCollector;


struct NodeInfo {
    opp_component_ptr<cModule> moduleRef;
    opp_component_ptr<LteMacBase> macModule;
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
    typedef std::set<MacNodeId> MulticastGroupIdSet;
    std::map<MacNodeId, MulticastGroupIdSet> nodeGroupMemberships_;
    std::set<MacNodeId> multicastTransmitterSet_;

    /*
     * Multicast destination ID support
     */
    // Counter for allocating new multicast destination IDs
    int16_t multicastDestIdCounter_ = MULTICAST_DEST_MIN_ID;
    // Mapping from IPv4 multicast addresses to allocated multicast destination IDs
    std::map<inet::Ipv4Address, MacNodeId> multicastAddrToDestId_;
    // Reverse mapping from multicast destination IDs to IPv4 addresses (optional, for debugging)
    std::map<MacNodeId, inet::Ipv4Address> multicastDestIdToAddr_;

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
    virtual bool isValidNodeId(MacNodeId  nodeId) const;
    virtual LteD2DMode computeD2DCapability(MacNodeId src, MacNodeId dst);

  public:
    Binder() {}

    virtual unsigned int getTotalBands()
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

    virtual std::string& getNetworkName()
    {
        return networkName_;
    }

    /**
     * Registers a carrier to the global Binder module
     */
    virtual void registerCarrier(GHz carrierFrequency, unsigned int carrierNumBands, unsigned int numerologyIndex,
            bool useTdd = false, unsigned int tddNumSymbolsDl = 0, unsigned int tddNumSymbolsUl = 0);

    /**
     * Registers a UE to a given carrier
     */
    virtual void registerCarrierUe(GHz carrierFrequency, unsigned int numerologyIndex, MacNodeId ueId);

    /**
     * Returns the set of UEs enabled on the given carrier
     */
    virtual const UeSet& getCarrierUeSet(GHz carrierFrequency);

    /**
     * Returns the max numerology index used by the given UE
     */
    virtual NumerologyIndex getUeMaxNumerologyIndex(MacNodeId ueId);

    /**
     * Returns the numerology indices used by the given UE
     */
    virtual const std::set<NumerologyIndex> *getUeNumerologyIndex(MacNodeId ueId);

    /**
     * Returns the numerology associated to a carrier frequency
     */
    virtual NumerologyIndex getNumerologyIndexFromCarrierFreq(GHz carrierFreq) { return carrierFreqToNumerologyIndex_[carrierFreq]; }

    /**
     * Returns the slot duration associated to the numerology (in seconds)
     */
    virtual double getSlotDurationFromNumerologyIndex(NumerologyIndex numerologyIndex) { return pow(2.0, (-1) * (int)numerologyIndex) / 1000.0; }

    /**
     * Compute slot format object given the number of DL and UL symbols
     */
    virtual SlotFormat computeSlotFormat(bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl);

    /**
     * Returns the slot format for the given carrier
     */
    virtual SlotFormat getSlotFormat(GHz carrierFrequency);

    /**
     * Registers a node to the global Binder module. If the nodeId is already occupied, an error will be thrown.
     * isNR specifies whether an LTE or 5G NR nodeId of a UE is to be assigned.
     */
    virtual void registerNode(MacNodeId nodeId, cModule *nodeModule, RanNodeType type, bool isNr = false);

    /**
     * Un-registers a node from the global Binder module.
     */
    virtual void unregisterNode(MacNodeId id);

    /**
     * Binds an UE with its serving eNodeB/gNodeB. Invoked at the start of
     * the simulation and on handovers.
     */
    virtual void registerServingNode(MacNodeId enbId, MacNodeId ueId);

    /**
     * Unregisters the UE from its current eNodeB/gNodeB. Invoked e.g. on handovers.
     */
    virtual void unregisterServingNode(MacNodeId enbId, MacNodeId ueId);

    /**
     * Returns the serving eNodeB/gNodeB of an UE, or NODEID_NONE if there is none.
     */
    virtual MacNodeId getServingNode(MacNodeId ueId);

    /**
     * registerMasterNode()
     *
     * It registers the secondary node to its master node (used for dual connectivity)
     *
     * @param masterId MacNodeId of the Master
     * @param slaveId MacNodeId of the Slave
     */
    virtual void registerMasterNode(MacNodeId masterId, MacNodeId slaveId);

    /**
     * Returns true if the node exists.
     */
    virtual bool nodeExists(MacNodeId nodeId) { return nodeInfoMap_.find(nodeId) != nodeInfoMap_.end(); }

    /**
     * Returns nullptr if not found.
     */
    virtual cModule *getNodeModule(MacNodeId nodeId);

    /*
     * getNodeInfoMap returns information on all nodes in a map
     */
    virtual const std::map<MacNodeId, NodeInfo>& getNodeInfoMap() const { return nodeInfoMap_; }

    /*
     * getMacFromMacNodeId() returns the reference to the LteMacBase module
     * given the MacNodeId of a node
     *
     * @param id MacNodeId of the module
     * @return LteMacBase* of the module
     */
    virtual LteMacBase *getMacFromMacNodeId(MacNodeId id);

    /**
     * For an UE, returns the serving eNodeB/gNodeB; for an eNodeB/gNodeB, returns the nodeId (the arg).
     */
    virtual MacNodeId getNextHop(MacNodeId nodeId);

    /**
     * In a Dual Connectivity / Split Bearer setup, returns the Master Node (MeNB, MN)
     * for the given Secondary Node (SeNB, SN).
     */
    //TODO add  MacNodeId ueId arg: In LTE DC, the roles (master/secondary) are per UE, not per bearer.
    virtual MacNodeId getMasterNodeOrSelf(MacNodeId secondaryEnbId);

    /**
     * TODO add "forUeId" argument, to getMasterNode() too
     */
    virtual MacNodeId getSecondaryNode(MacNodeId masterEnbId);

    /**
     * Returns the MacNodeId for the given IP address
     *
     * @param address IP address
     * @return MacNodeId corresponding to the IP address
     */
    virtual MacNodeId getMacNodeId(inet::Ipv4Address address)
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
    virtual MacNodeId getNrMacNodeId(inet::Ipv4Address address)
    {
        if (ipAddressToNrMacNodeId_.find(address) == ipAddressToNrMacNodeId_.end())
            return NODEID_NONE;
        return ipAddressToNrMacNodeId_[address];
    }

    /**
     * Returns the selected nodeId (LTE/NR) for the given UE.
     *
     * @param ue The UE's current MacNodeId (can be either LTE or NR)
     * @param isNr If true, returns the NR nodeId; if false, returns the LTE nodeId
     * @return The requested nodeId, or NODEID_NONE if not found/available
     */
    virtual MacNodeId getUeNodeId(MacNodeId ue, bool isNr);

    /**
     * author Alessandro Noferi
     *
     * Returns the IP address for the given MacNodeId
     *
     * @param MacNodeId of the node
     * @return IP address corresponding to the MacNodeId
     *
     */
    virtual inet::Ipv4Address getIPv4Address(MacNodeId nodeId)
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
    virtual X2NodeId getX2NodeId(inet::Ipv4Address address)
    {
        return getMacNodeId(address);
    }

    /**
     * Associates the given IP address with the given MacNodeId.
     *
     * @param address IP address
     */
    virtual void setMacNodeId(inet::Ipv4Address address, MacNodeId nodeId)
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
    virtual void setX2NodeId(inet::Ipv4Address address, X2NodeId nodeId)
    {
        setMacNodeId(address, nodeId);
    }

    virtual inet::L3Address getX2PeerAddress(X2NodeId srcId, X2NodeId destId)
    {
        return x2PeerAddress_[srcId][destId];
    }

    virtual void setX2PeerAddress(X2NodeId srcId, X2NodeId destId, inet::L3Address interfAddr)
    {
        x2PeerAddress_[srcId].insert({destId, interfAddr});
    }

    /**
     * Register the address of MEC Hosts in the simulation
     */
    virtual void registerMecHost(const inet::L3Address& mecHostAddress);
    /**
     * Associates the given MEC Host address to its corresponding UPF
     */
    virtual void registerMecHostUpfAddress(const inet::L3Address& mecHostAddress, const inet::L3Address& gtpAddress);
    /**
     * Returns true if the given address belongs to a MEC host
     */
    virtual bool isMecHost(const inet::L3Address& mecHostAddress);
    /**
     * Returns the UPF address corresponding to the given MEC Host address
     */
    virtual const inet::L3Address& getUpfFromMecHost(const inet::L3Address& mecHostAddress);
    /**
     * Returns the module for the given MAC node ID
     */
    virtual cModule *getModuleByMacNodeId(MacNodeId nodeId);

    /*
     * getDeployedUes() returns the affiliates
     * of a given eNodeB
     */
    virtual std::vector<MacNodeId> getDeployedUes(MacNodeId enbNodeId);

    PhyPisaData phyPisaData;

    virtual int getNodeCount() {
        return nodeInfoMap_.size();
    }

    virtual int addExtCell(ExtCell *extCell, GHz carrierFrequency)
    {
        if (extCellList_.find(carrierFrequency) == extCellList_.end())
            extCellList_[carrierFrequency] = ExtCellList();

        extCellList_[carrierFrequency].push_back(extCell);
        return extCellList_[carrierFrequency].size() - 1;
    }

    virtual ExtCellList getExtCellList(GHz carrierFrequency)
    {
        return extCellList_[carrierFrequency];
    }

    virtual int addBackgroundScheduler(BackgroundScheduler *bgScheduler, GHz carrierFrequency)
    {
        if (bgSchedulerList_.find(carrierFrequency) == bgSchedulerList_.end())
            bgSchedulerList_[carrierFrequency] = BackgroundSchedulerList();

        bgSchedulerList_[carrierFrequency].push_back(bgScheduler);
        return bgSchedulerList_[carrierFrequency].size() - 1;
    }

    virtual const BackgroundSchedulerList& getBackgroundSchedulerList(GHz carrierFrequency)
    {
        return bgSchedulerList_[carrierFrequency];
    }

    virtual void addEnbInfo(EnbInfo *info)
    {
        enbList_.push_back(info);
    }

    virtual const std::vector<EnbInfo *>& getEnbList()
    {
        return enbList_;
    }

    virtual void addUeInfo(UeInfo *info)
    {
        ueList_.push_back(info);
    }

    virtual const std::vector<UeInfo *>& getUeList()
    {
        return ueList_;
    }

    virtual void addBgTrafficManagerInfo(BgTrafficManagerInfo *info)
    {
        bgTrafficManagerList_.push_back(info);
    }

    virtual const std::vector<BgTrafficManagerInfo *>& getBgTrafficManagerList()
    {
        return bgTrafficManagerList_;
    }

    virtual Cqi meanCqi(std::vector<Cqi> bandCqi, MacNodeId id, Direction dir);

    virtual Cqi medianCqi(std::vector<Cqi> bandCqi, MacNodeId id, Direction dir);

    /*
     * Uplink interference support
     */
    virtual simtime_t getLastUpdateUlTransmissionInfo();
    virtual void initAndResetUlTransmissionInfo();
    virtual void storeUlTransmissionMap(GHz carrierFreq, Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, LtePhyBase *phy, Direction dir);
    virtual void storeUlTransmissionMap(GHz carrierFreq, Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, TrafficGeneratorBase *trafficGen, Direction dir);  // overloaded function for bgUes
    virtual const std::vector<std::vector<UeAllocationInfo>> *getUlTransmissionMap(GHz carrierFreq, UlTransmissionMapTTI t);
    /*
     * X2 Support
     */
    virtual void registerX2Port(X2NodeId nodeId, int port);
    virtual int getX2Port(X2NodeId nodeId);

    /*
     * D2D Support
     */
    virtual bool checkD2DCapability(MacNodeId src, MacNodeId dst);
    virtual bool getD2DCapability(MacNodeId src, MacNodeId dst);

    virtual std::map<MacNodeId, std::map<MacNodeId, LteD2DMode>> *getD2DPeeringMap();
    virtual LteD2DMode getD2DMode(MacNodeId src, MacNodeId dst);
    virtual bool isFrequencyReuseEnabled(MacNodeId nodeId);

    /*
     * Multicast Support
     */
    // add the group to the set of multicast group of nodeId
    virtual void joinMulticastGroup(MacNodeId nodeId, MacNodeId multicastDestId);
    // check if the node is enrolled in the group
    virtual bool isInMulticastGroup(MacNodeId nodeId, MacNodeId multicastDestId);
    // add one multicast transmitter
    virtual void addD2DMulticastTransmitter(MacNodeId nodeId);
    // get multicast transmitters
    virtual std::set<MacNodeId>& getD2DMulticastTransmitters();

    /*
     * Multicast Destination ID Support
     */
    // Allocate an (MBMS-RNTI-like) multicast destination ID for a multicast IPv4 address
    virtual MacNodeId getOrAssignDestIdForMulticastAddress(inet::Ipv4Address multicastAddr);
    // Checks if a multicast destination ID was already assigned for a multicast IPv4 address
    virtual bool hasMulticastDestIdAssigned(inet::Ipv4Address multicastAddr) const {
        return multicastAddrToDestId_.find(multicastAddr) != multicastAddrToDestId_.end();
    }
    // Get existing multicast destination ID for a multicast address (returns NODEID_NONE if not found)
    virtual MacNodeId getDestIdForMulticastAddress(inet::Ipv4Address multicastAddr);
    // Get multicast address from destination ID
    virtual inet::Ipv4Address getAddressForMulticastDestId(MacNodeId multicastDestId);

    /*
     *  Handover support
     */
    virtual void addUeHandoverTriggered(MacNodeId nodeId);
    virtual bool hasUeHandoverTriggered(MacNodeId nodeId);
    virtual void removeUeHandoverTriggered(MacNodeId nodeId);

    virtual void addHandoverTriggered(MacNodeId nodeId, MacNodeId srcId, MacNodeId destId);
    virtual const std::pair<MacNodeId, MacNodeId> *getHandoverTriggered(MacNodeId nodeId);
    virtual void removeHandoverTriggered(MacNodeId nodeId);

    virtual void updateUeInfoCellId(MacNodeId nodeId, MacCellId cellId);


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
    virtual void addUeCollectorToEnodeB(MacNodeId ue, UeStatsCollector *ueCollector, MacCellId cell);

    /* this method moves the UeStatsCollector reference between the eNB/gNB's baseStationStatsCollector
     * structure.
     * @params:
     *  ue: MacNodeId of the ue
     *  oldCell: MacCellId of the source eNB
     *  newCell: MacCellId of the target eNB
     */
    virtual void moveUeCollector(MacNodeId ue, MacCellId oldCell, MacCellId newCell);

    virtual bool isGNodeB(MacNodeId enbId);

    // Moved from LteCommon - getter functions that were taking Binder as first parameter
    virtual CellInfo *getCellInfoByNodeId(MacNodeId nodeId);
    virtual cModule *getPhyByNodeId(MacNodeId nodeId);
    virtual cModule *getMacByNodeId(MacNodeId nodeId);
    virtual cModule *getRlcByNodeId(MacNodeId nodeId, LteRlcType rlcType);
    virtual cModule *getRrcByNodeId(MacNodeId nodeId);

    // SMF-like Session Management Functions
    virtual void establishUnidirectionalDataConnection(FlowControlInfo *lteInfo);

  private:
    virtual bool isDualConnectivityRequired(FlowControlInfo *info);
    virtual void createConnection(FlowControlInfo *lteInfo, bool withPdcp);
    virtual void createIncomingConnectionOnNode(MacNodeId nodeId, FlowControlInfo *lteInfo, bool withPdcp);
    virtual void createOutgoingConnectionOnNode(MacNodeId nodeId, FlowControlInfo *lteInfo, bool withPdcp);
};

} //namespace

#endif
