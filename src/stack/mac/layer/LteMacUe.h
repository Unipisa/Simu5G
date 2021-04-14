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

#ifndef _LTE_LTEMACUE_H_
#define _LTE_LTEMACUE_H_

#include "stack/mac/layer/LteMacBase.h"
#include "stack/phy/layer/LtePhyBase.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/phy/feedback/LteFeedback.h"

class LteSchedulingGrant;
class LteSchedulerUeUl;
class Binder;
class LteChannelModel;

class LteMacUe : public LteMacBase
{
  protected:
    // false if currentHarq_ counter needs to be initialized
    bool firstTx;

    // one per carrier
    std::map<double, LteSchedulerUeUl*> lcgScheduler_;

    // configured grant - one each codeword
    std::map<double, inet::IntrusivePtr<const LteSchedulingGrant> > schedulingGrant_;

    /// List of scheduled connection for this UE
    std::map<double, LteMacScheduleList*> scheduleList_;

    // current H-ARQ process counter
    unsigned char currentHarq_;

    // perodic grant handling - one per carrier
    std::map<double, unsigned int> periodCounter_;
    std::map<double, unsigned int> expirationCounter_;

    // number of MAC SDUs requested to the RLC
    int requestedSdus_;

    bool debugHarq_;

    // RAC Handling variables

    bool racRequested_;
    unsigned int racBackoffTimer_;
    unsigned int maxRacTryouts_;
    unsigned int currentRacTry_;
    unsigned int minRacBackoff_;
    unsigned int maxRacBackoff_;

    unsigned int raRespTimer_;
    unsigned int raRespWinStart_;

    // BSR handling
    bool bsrTriggered_;

//    // reference to the channel models
//    std::map<double, LteChannelModel*> channelModel_;

    // statistics
    omnetpp::simsignal_t cqiDlSpmux0_;
    omnetpp::simsignal_t cqiDlSpmux1_;
    omnetpp::simsignal_t cqiDlSpmux2_;
    omnetpp::simsignal_t cqiDlSpmux3_;
    omnetpp::simsignal_t cqiDlSpmux4_;
    omnetpp::simsignal_t cqiDlTxDiv0_;
    omnetpp::simsignal_t cqiDlTxDiv1_;
    omnetpp::simsignal_t cqiDlTxDiv2_;
    omnetpp::simsignal_t cqiDlTxDiv3_;
    omnetpp::simsignal_t cqiDlTxDiv4_;
    omnetpp::simsignal_t cqiDlMuMimo0_;
    omnetpp::simsignal_t cqiDlMuMimo1_;
    omnetpp::simsignal_t cqiDlMuMimo2_;
    omnetpp::simsignal_t cqiDlMuMimo3_;
    omnetpp::simsignal_t cqiDlMuMimo4_;
    omnetpp::simsignal_t cqiDlSiso0_;
    omnetpp::simsignal_t cqiDlSiso1_;
    omnetpp::simsignal_t cqiDlSiso2_;
    omnetpp::simsignal_t cqiDlSiso3_;
    omnetpp::simsignal_t cqiDlSiso4_;

    /**
     * Reads MAC parameters for ue and performs initialization.
     */
    virtual void initialize(int stage) override;

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /**
     * macSduRequest() sends a message to the RLC layer
     * requesting MAC SDUs (one for each CID),
     * according to the Schedule List.
     */
    virtual int macSduRequest();

    /**
     * bufferizePacket() is called every time a packet is
     * received from the upper layer
     */
    virtual bool bufferizePacket(omnetpp::cPacket* pkt) override;

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List.
     * It sends them to H-ARQ (at the moment lower layer)
     *
     * On UE it also adds a BSR control element to the MAC PDU
     * containing the size of its buffer (for that CID)
     */
    virtual void macPduMake(MacCid cid = 0) override;

    /**
     * macPduUnmake() extracts SDUs from a received MAC
     * PDU and sends them to the upper layer.
     *
     * @param pkt container packet
     */
    virtual void macPduUnmake(omnetpp::cPacket* pkt) override;

    /**
     * handleUpperMessage() is called every time a packet is
     * received from the upper layer
     */
    virtual void handleUpperMessage(omnetpp::cPacket* pkt) override;

    /**
     * Main loop
     */
    virtual void handleSelfMessage() override;

    /*
     * Receives and handles scheduling grants
     */
    virtual void macHandleGrant(omnetpp::cPacket* pkt) override;

    /*
     * Receives and handles RAC responses
     */
    virtual void macHandleRac(omnetpp::cPacket* pkt) override;

    /*
     * Checks RAC status
     */
    virtual void checkRAC();
    /*
     * Update UserTxParam stored in every lteMacPdu when an rtx change this information
     */
    virtual void updateUserTxParam(omnetpp::cPacket* pkt) override;

    /**
     * Flush Tx H-ARQ buffers for the user
     */
    virtual void flushHarqBuffers();

  public:
    LteMacUe();
    virtual ~LteMacUe();

    /*
     * Access scheduling grant
     */
    inline const LteSchedulingGrant* getSchedulingGrant(double carrierFrequency) const
    {
        if (schedulingGrant_.find(carrierFrequency) == schedulingGrant_.end())
            return NULL;
        return schedulingGrant_.at(carrierFrequency).get();
    }
    /*
     * Access current H-ARQ pointer
     */
    inline const unsigned char getCurrentHarq() const
    {
        return currentHarq_;
    }
    /*
     * Access BSR trigger flag
     */
    inline const bool bsrTriggered() const
    {
        return bsrTriggered_;
    }

    /* utility functions used by LCP scheduler
     * <cid> and <priority> are returned by reference
     * @return true if at least one backlogged connection exists
     */
    bool getHighestBackloggedFlow(MacCid& cid, unsigned int& priority);
    bool getLowestBackloggedFlow(MacCid& cid, unsigned int& priority);

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    virtual void deleteQueues(MacNodeId nodeId) override;

    // update ID of the serving cell during handover
    virtual void doHandover(MacNodeId targetEnb);
};

#endif
