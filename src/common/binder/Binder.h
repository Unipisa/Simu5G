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

#include <omnetpp.h>
#include <string>

#include <inet/networklayer/contract/ipv4/Ipv4Address.h>
#include <inet/networklayer/common/L3Address.h>
#include "common/LteCommon.h"
#include "common/blerCurves/PhyPisaData.h"
#include "nodes/ExtCell.h"
#include "stack/mac/layer/LteMacBase.h"

/**
 * The Binder module has one instance in the whole network.
 * It stores global mapping tables with OMNeT++ module IDs,
 * IP addresses, etc.
 */
class Binder : public omnetpp::cSimpleModule
{
  private:
    typedef std::map<MacNodeId, std::map<MacNodeId, bool> > DeployedUesMap;

    std::map<inet::Ipv4Address, MacNodeId> macNodeIdToIPAddress_;
    std::map<inet::Ipv4Address, MacNodeId> nrMacNodeIdToIPAddress_;
    std::map<MacNodeId, char*> macNodeIdToModuleName_;
    std::map<MacNodeId, LteMacBase*> macNodeIdToModule_;
    std::vector<MacNodeId> nextHop_; // MacNodeIdMaster --> MacNodeIdSlave
    std::vector<MacNodeId> secondaryNodeToMasterNode_;
    std::map<int, OmnetId> nodeIds_;

    // list of static external cells. Used for intercell interference evaluation
    std::map<double, ExtCellList> extCellList_;

    // list of all eNBs. Used for inter-cell interference evaluation
    std::vector<EnbInfo*> enbList_;

    // list of all UEs. Used for inter-cell interference evaluation
    std::vector<UeInfo*> ueList_;

    MacNodeId macNodeIdCounter_[3]; // MacNodeId Counter
    DeployedUesMap dMap_; // DeployedUes --> Master Mapping

    /*
     * Carrier Aggregation support
     */
    // total number of logical bands in the system. These bands
    unsigned int totalBands_;
    CarrierInfoMap componentCarriers_;

    // for each carrier, store the UEs that are able to use it
    typedef std::map<double, UeSet > CarrierUeMap;
    CarrierUeMap carrierUeMap_;


    // hack to link carrier freq to numerology index
    std::map<double, NumerologyIndex> carrierFreqToNumerologyIndex_;
    // max numerology index used by UEs
    std::vector<NumerologyIndex> ueMaxNumerologyIndex_;
    // set of numerologies used by each UE
    std::map<MacNodeId, std::set<NumerologyIndex> > ueNumerologyIndex_;

    /*
     * Uplink interference support
     */
    typedef std::map<double, std::vector< std::vector< std::vector<UeAllocationInfo> > > > UplinkTransmissionMap;
    // for each carrier frequency, for both previous and current TTIs, for each RB, stores the UE (nodeId and ref to the PHY module) that transmitted/are transmitting within that RB
    UplinkTransmissionMap ulTransmissionMap_;
    // TTI of the last update of the UL band status
    omnetpp::simtime_t lastUpdateUplinkTransmissionInfo_;

    /*
     * X2 Support
     */
    typedef std::map<X2NodeId, std::list<int> > X2ListeningPortMap;
    X2ListeningPortMap x2ListeningPorts_;

    std::map<MacNodeId, std::map<MacNodeId, inet::L3Address> > x2PeerAddress_;

//    std::map<MacNodeId, L3Address> x2Address_;

    /*
     * D2D Support
     */
    // determines if two D2D-capable UEs are communicating in D2D mode or Infrastructure Mode
    std::map<MacNodeId, std::map<MacNodeId, LteD2DMode> > d2dPeeringMap_;

    /*
     * Multicast support
     */
    // register here the IDs of the multicast group where UEs participate
    typedef std::set<inet::uint32> MulticastGroupIdSet;
    std::map<MacNodeId, MulticastGroupIdSet> multicastGroupMap_;
    std::set<MacNodeId> multicastTransmitterSet_;

    /*
     * Handover support
     */
    // store the id of the UEs that are performing handover
    std::set<MacNodeId> ueHandoverTriggered_;
    std::map<MacNodeId, std::pair<MacNodeId, MacNodeId> > handoverTriggered_;
  protected:
    virtual void initialize(int stages) override;
    virtual int numInitStages() const override { return inet::INITSTAGE_LAST; }
    virtual void handleMessage(omnetpp::cMessage *msg) override
    {
    }

  public:
    Binder()
    {
        macNodeIdCounter_[0] = ENB_MIN_ID;
        macNodeIdCounter_[1] = UE_MIN_ID;
        macNodeIdCounter_[2] = NR_UE_MIN_ID;

        totalBands_ = 0;
    }

    unsigned int getTotalBands()
    {
        return totalBands_;
    }

    virtual ~Binder()
    {
        while(enbList_.size() > 0){
            delete enbList_.back();
            enbList_.pop_back();
        }

        for (auto it = macNodeIdToModuleName_.begin(); it != macNodeIdToModuleName_.end(); ++it)
            delete[] it->second;

        for (auto it = ueList_.begin(); it != ueList_.end(); ++it)
            delete (*it);
        ueList_.clear();
    }

    /**
     * Registers a carrier to the global Binder module
     */
    void registerCarrier(double carrierFrequency, unsigned int carrierNumBands, unsigned int numerologyIndex,
            bool useTdd=false, unsigned int tddNumSymbolsDl=0, unsigned int tddNumSymbolsUl=0);

    /**
     * Registers a UE to a given carrier
     */
    void registerCarrierUe(double carrierFrequency, unsigned int numerologyIndex, MacNodeId ueId);

    /**
     * Returns the set of UEs enabled on the given carrier
     */
    const UeSet& getCarrierUeSet(double carrierFrequency);

    /**
     * Returns the max numerology index used by the given UE
     */
    NumerologyIndex getUeMaxNumerologyIndex(MacNodeId ueId);

    /**
     * Returns the numerology indices used by the given UE
     */
    const std::set<NumerologyIndex>* getUeNumerologyIndex(MacNodeId ueId);

    /**
     * Returns the numerology associated to a carrier frequency
     */
    NumerologyIndex getNumerologyIndexFromCarrierFreq(double carrierFreq) { return carrierFreqToNumerologyIndex_[carrierFreq]; }

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
    SlotFormat getSlotFormat(double carrierFrequency);

    /**
     * Registers a node to the global Binder module.
     *
     * The binder assigns an IP address to the node, from which it is derived
     * an unique macNodeId.
     * The node registers its moduleId (omnet ID), and if it's a UE,
     * it registers also the association with its master node.
     *
     * @param module pointer to the module to be registered
     * @param type type of this node (ENODEB, GNODEB, UE)
     * @param masterId id of the master of this node, 0 if none (node is an eNB)
     * @return macNodeId assigned to the module
     */
    MacNodeId registerNode(cModule *module, RanNodeType type, MacNodeId masterId = 0, bool registerNr = false);

    /**
     * Un-registers a node from the global Binder module.
     */
    void unregisterNode(MacNodeId id);

    /**
     * registerNextHop() is called by the IP2NIC module at network startup
     * to bind each slave with its masters. It is also
     * called on handovers to synchronize the nextHop table:
     *
     * It registers a slave to its current master
     *
     * @param masterId MacNodeId of the Master
     * @param slaveId MacNodeId of the Slave
     */
    void registerNextHop(MacNodeId masterId, MacNodeId slaveId);

    /**
     * registerNextHop() is called on handovers to sychronize
     * the nextHop table:
     *
     * It unregisters the slave from its old master
     *
     * @param masterId MacNodeId of the Master
     * @param slaveId MacNodeId of the Slave
     */
    void unregisterNextHop(MacNodeId masterId, MacNodeId slaveId);

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
     * getOmnetId() returns the Omnet Id of the module
     * given its MacNodeId
     *
     * @param nodeId MacNodeId of the module
     * @return OmnetId of the module
     */
    OmnetId getOmnetId(MacNodeId nodeId);

    /*
     * get iterators for the list of nodes
     */
    std::map<int, OmnetId>::const_iterator getNodeIdListBegin();
    std::map<int, OmnetId>::const_iterator getNodeIdListEnd();

    /**
     * getMacNodeIdFromOmnetId() returns the MacNodeId of the module
     * given its OmnetId
     *
     * @param id OmnetId of the module
     * @return MacNodeId of the module
     */
    MacNodeId getMacNodeIdFromOmnetId(OmnetId id);

    /*
     * getMacFromMacNodeId() returns the reference to the LteMacBase module
     * given the MacNodeId of a node
     *
     * @param id MacNodeId of the module
     * @return LteMacBase* of the module
     */
    LteMacBase* getMacFromMacNodeId(MacNodeId id);

    /**
     * getNextHop() returns the master of
     * a given slave
     *
     * @param slaveId MacNodeId of the Slave
     * @return MacNodeId of the master
     */
    MacNodeId getNextHop(MacNodeId slaveId);

    /**
     * getMasterNode() returns the master of
     * a given slave (used for dual connectivity)
     *
     * @param slaveId MacNodeId of the Slave
     * @return MacNodeId of the master
     */
    MacNodeId getMasterNode(MacNodeId slaveId);

    /**
     * Returns the MacNodeId for the given IP address
     *
     * @param address IP address
     * @return MacNodeId corresponding to the IP addres
     */
    MacNodeId getMacNodeId(inet::Ipv4Address address)
    {
        if (macNodeIdToIPAddress_.find(address) == macNodeIdToIPAddress_.end())
            return 0;

        MacNodeId nodeId = macNodeIdToIPAddress_[address];
        // if the UE is disconnected (its master node is 0), check the NR node Id
        if (getNextHop(nodeId) == 0)
            return getNrMacNodeId(address);
        return nodeId;
    }
    /**
     * Returns the MacNodeId for the given IP address
     *
     * @param address IP address
     * @return MacNodeId corresponding to the IP addres
     */
    MacNodeId getNrMacNodeId(inet::Ipv4Address address)
    {
        if (nrMacNodeIdToIPAddress_.find(address) == nrMacNodeIdToIPAddress_.end())
            return 0;
        return nrMacNodeIdToIPAddress_[address];
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
            nrMacNodeIdToIPAddress_[address] = nodeId;
        else
            macNodeIdToIPAddress_[address] = nodeId;
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
        std::pair<X2NodeId, inet::L3Address> p(destId, interfAddr);
        x2PeerAddress_[srcId].insert(p);
    }
//    L3Address getX2Address(X2NodeId nodeId)
//    {
//        return x2Address_[nodeId];
//    }
//    void setX2Address(X2NodeId nodeId, L3Address interfAddr)
//    {
//        if (x2Address_.find(nodeId) != x2Address_.end())
//            return;
//        x2Address_[nodeId] = interfAddr;
//    }
    /**
     * Associates the given MAC node ID to the name of the module
     */
    void registerName(MacNodeId nodeId, const char* moduleName);
    /**
     * Returns the module name for the given MAC node ID
     */
    const char* getModuleNameByMacNodeId(MacNodeId nodeId);

    /*
     * getDeployedUes() returns the affiliates
     * of a given eNodeB
     */
    ConnectedUesMap getDeployedUes(MacNodeId localId, Direction dir);
    PhyPisaData phyPisaData;

    int getNodeCount(){
        return nodeIds_.size();
    }

    int addExtCell(ExtCell* extCell, double carrierFrequency)
    {
        if (extCellList_.find(carrierFrequency) == extCellList_.end())
            extCellList_[carrierFrequency] = ExtCellList();

        extCellList_[carrierFrequency].push_back(extCell);
        return extCellList_[carrierFrequency].size() - 1;
    }

    ExtCellList getExtCellList(double carrierFrequency)
    {
        return extCellList_[carrierFrequency];
    }

    void addEnbInfo(EnbInfo* info)
    {
        enbList_.push_back(info);
    }

    std::vector<EnbInfo*> * getEnbList()
    {
        return &enbList_;
    }

    void addUeInfo(UeInfo* info)
    {
        ueList_.push_back(info);
    }

    std::vector<UeInfo*> * getUeList()
    {
        return &ueList_;
    }

    Cqi meanCqi(std::vector<Cqi> bandCqi,MacNodeId id,Direction dir);

    Cqi medianCqi(std::vector<Cqi> bandCqi,MacNodeId id,Direction dir);

    /*
     * Uplink interference support
     */
    omnetpp::simtime_t getLastUpdateUlTransmissionInfo();
    void initAndResetUlTransmissionInfo();
    void storeUlTransmissionMap(double carrierFreq, Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, LtePhyBase* phy, Direction dir);
    const std::vector<UeAllocationInfo>* getUlTransmissionMap(double carrierFreq, UlTransmissionMapTTI t, Band b);
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

    std::map<MacNodeId, std::map<MacNodeId, LteD2DMode> >* getD2DPeeringMap();
    void setD2DMode(MacNodeId src, MacNodeId dst, LteD2DMode mode);
    LteD2DMode getD2DMode(MacNodeId src, MacNodeId dst);
    bool isFrequencyReuseEnabled(MacNodeId nodeId);
    /*
     * Multicast Support
     */
    // add the group to the set of multicast group of nodeId
    void registerMulticastGroup(MacNodeId nodeId, inet::int32 groupId);
    // check if the node is enrolled in the group
    bool isInMulticastGroup(MacNodeId nodeId, inet::int32 groupId);
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
    const std::pair<MacNodeId, MacNodeId>* getHandoverTriggered(MacNodeId nodeId);
    void removeHandoverTriggered(MacNodeId nodeId);

    void updateUeInfoCellId(MacNodeId nodeId, MacCellId cellId);
};

#endif
