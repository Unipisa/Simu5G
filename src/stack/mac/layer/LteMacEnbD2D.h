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

#ifndef _LTE_LTEMACENBD2D_H_
#define _LTE_LTEMACENBD2D_H_

#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferMirrorD2D.h"
#include "stack/d2dModeSelection/D2DModeSwitchNotification_m.h"
#include "stack/mac/conflict_graph/ConflictGraph.h"

typedef std::pair<MacNodeId, MacNodeId> D2DPair;
typedef std::map<D2DPair, LteHarqBufferMirrorD2D*> HarqBuffersMirrorD2D;
class ConflictGraph;

class LteMacEnbD2D : public LteMacEnb
{
  protected:

    /*
     * Stores the mirrored status of H-ARQ buffers for D2D transmissions.
     * The key value of the map is the pair <sender,receiver> of the D2D flow
     */
    std::map<double, HarqBuffersMirrorD2D> harqBuffersMirrorD2D_;

    // if true, use the preconfigured TX params for transmission, else use that signaled by the eNB
    bool usePreconfiguredTxParams_;
    UserTxParams* preconfiguredTxParams_;

    // Conflict Graph builder
    ConflictGraph* conflictGraph_;

    // parameters for conflict graph (needed when frequency reuse is enabled)
    bool reuseD2D_;
    bool reuseD2DMulti_;

    omnetpp::simtime_t conflictGraphUpdatePeriod_;
    double conflictGraphThreshold_;

    // handling of D2D mode switch
    bool msHarqInterrupt_;   // if true, H-ARQ processes of D2D flows are interrupted at mode switch
                             // otherwise, they are terminated using the old communication mode
    bool msClearRlcBuffer_;  // if true, SDUs stored in the RLC buffer of D2D flows are dropped

    void clearBsrBuffers(MacNodeId ueId);

    /**
     * macPduUnmake() extracts SDUs from a received MAC
     * PDU and sends them to the upper layer.
     *
     * On ENB it also extracts the BSR Control Element
     * and stores it in the BSR buffer (for the cid from
     * which packet was received)
     *
     * @param pkt container packet
     */
    virtual void macPduUnmake(omnetpp::cPacket* pkt);

    virtual void macHandleFeedbackPkt(omnetpp::cPacket *pkt);
    /**
     * creates scheduling grants (one for each nodeId) according to the Schedule List.
     * It sends them to the  lower layer
     */
    virtual void sendGrants(std::map<double, LteMacScheduleList>* scheduleList);

    void macHandleD2DModeSwitch(omnetpp::cPacket* pkt);

    /**
     * Flush Tx H-ARQ buffers for all users
     */
    virtual void flushHarqBuffers();

    /// Lower Layer Handler
    virtual void fromPhy(omnetpp::cPacket *pkt);

  public:

    LteMacEnbD2D();
    virtual ~LteMacEnbD2D();

    /**
     * Reads MAC parameters for ue and performs initialization.
     */
    virtual void initialize(int stage);

    /**
     * Main loop
     */
    virtual void handleSelfMessage();

    virtual void handleMessage(omnetpp::cMessage* msg);

    virtual bool isD2DCapable()
    {
        return true;
    }

    virtual bool isReuseD2DEnabled()
    {
        return reuseD2D_;
    }

    virtual bool isReuseD2DMultiEnabled()
    {
        return reuseD2DMulti_;
    }

    virtual ConflictGraph* getConflictGraph()
    {
        return conflictGraph_;
    }

    /**
     * deleteQueues() on ENB performs actions
     * from base classes and also deletes mirror buffers
     *
     * @param nodeId id of node performig handover
     */
    virtual void deleteQueues(MacNodeId nodeId);

    // get the reference to the "mirror" buffers
    HarqBuffersMirrorD2D* getHarqBuffersMirrorD2D(double carrierFrequency);

    // delete the "mirror" Harq Buffer for this pair (useful at mode switch)
    void deleteHarqBuffersMirrorD2D(MacNodeId txPeer, MacNodeId rxPeer);
    // delete the "mirror" Harq Buffer for this node (useful at handover)
    void deleteHarqBuffersMirrorD2D(MacNodeId nodeId);

    // send the D2D Mode Switch signal to the transmitter of the given flow
    void sendModeSwitchNotification(MacNodeId srcId, MacNodeId dst, LteD2DMode oldMode, LteD2DMode newMode);

    bool isMsHarqInterrupt() { return msHarqInterrupt_; }

    /*
     * getter
     */
    UserTxParams* getPreconfiguredTxParams(){

        return preconfiguredTxParams_;
    }
};

#endif
