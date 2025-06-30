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

#ifndef _LTE_LTEMACUED2D_H_
#define _LTE_LTEMACUED2D_H_

#include "stack/mac/LteMacUe.h"
#include "stack/mac/LteMacEnbD2D.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferTxD2D.h"

namespace simu5g {

using namespace omnetpp;

class LteSchedulingGrant;
class LteSchedulerUeUl;
class Binder;

class LteMacUeD2D : public LteMacUe
{

  protected:

    // reference to the eNB
    opp_component_ptr<LteMacEnbD2D> enb_;

    // flag for empty schedule list (true when no carriers have been scheduled)
    bool emptyScheduleList_;

    // RAC Handling variables
    bool racD2DMulticastRequested_ = false;
    // Multicast D2D BSR handling
    bool bsrD2DMulticastTriggered_ = false;

    static simsignal_t rcvdD2DModeSwitchNotificationSignal_;

    // if true, use the preconfigured TX params for transmission, else use those signaled by the eNB
    bool usePreconfiguredTxParams_;
    UserTxParams *preconfiguredTxParams_ = nullptr;
    UserTxParams *getPreconfiguredTxParams();  // build and return new user tx params

    /**
     * Reads MAC parameters for the UE and performs initialization.
     */
    void initialize(int stage) override;

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    void handleMessage(cMessage *msg) override;

    /**
     * Main loop
     */
    void handleSelfMessage() override;

    void macHandleGrant(cPacket *pkt) override;

    /*
     * Checks RAC status
     */
    void checkRAC() override;

    /*
     * Receives and handles RAC responses
     */
    void macHandleRac(cPacket *pkt) override;

    void macHandleD2DModeSwitch(cPacket *pkt);

    virtual Packet *makeBsr(int size);

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List.
     * It sends them to H-ARQ (at the moment lower layer)
     *
     * On UE it also adds a BSR control element to the MAC PDU
     * containing the size of its buffer (for that CID)
     */
    void macPduMake(MacCid cid = 0) override;

  public:
    ~LteMacUeD2D() override;

    bool isD2DCapable() override
    {
        return true;
    }

    virtual void triggerBsr(MacCid cid)
    {
        if (connDesc_[cid].getDirection() == D2D_MULTI)
            bsrD2DMulticastTriggered_ = true;
        else
            bsrTriggered_ = true;
    }

    void doHandover(MacNodeId targetEnb) override;
};

} //namespace

#endif

