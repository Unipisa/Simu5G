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

#ifndef _LTE_LTE_SCHEDULER_ENB_UL_H_
#define _LTE_LTE_SCHEDULER_ENB_UL_H_

#include "stack/mac/scheduler/LteSchedulerEnb.h"

/**
 * @class LteSchedulerEnbUl
 *
 * LTE eNB uplink scheduler.
 */
class LteSchedulerEnbUl : public LteSchedulerEnb
{
  protected:

    typedef std::map<MacNodeId, unsigned char> HarqStatus;
    typedef std::map<MacNodeId, bool> RacStatus;

    /// Minimum scheduling unit, represents the MAC SDU size
    unsigned int scheduleUnit_;
    //---------------------------------------------

    /**
     * Checks Harq Descriptors and return the first free codeword.
     *
     * @param id
     * @param cw
     * @return
     */
    virtual bool checkEligibility(MacNodeId id, Codeword& cw, double carrierFrequency);

    //! Uplink Synchronous H-ARQ process counter - keeps track of currently active process on connected UES.
    std::map<double, HarqStatus> harqStatus_;

    //! RAC requests flags: signals wheter an UE shall be granted the RAC allocation
    RacStatus racStatus_;

  public:

    //! Updates HARQ descriptor current process pointer (to be called every TTI by main loop).
    virtual void updateHarqDescs();

    /**
     * Updates current schedule list with RAC grant responses.
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool racschedule(double carrierFrequency);

    /**
     * Updates current schedule list with HARQ retransmissions.
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool rtxschedule(double carrierFrequency, BandLimitVector* bandLim = NULL);

    /**
     * signals RAC request to the scheduler (called by eNb)
     */
    virtual void signalRac(const MacNodeId nodeId)
    {
        racStatus_[nodeId] = true;
    }

    /**
     * Schedules retransmission for the Harq Process of the given UE on a set of logical bands.
     * Each band has also assigned a band limit amount of bytes: no more than the specified
     * amount will be served on the given band for the acid.
     *
     * @param nodeId The node ID
     * @param carrierFrequency carrier frequency
     * @param cw The codeword used to serve the acid process
     * @param bands A vector of logical bands
     * @param acid The ACID
     * @return The allocated bytes. 0 if retransmission was not possible
     */
    virtual unsigned int schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
        std::vector<BandLimit>* bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false);

    unsigned int schedulePerAcidRtxD2D(MacNodeId destId, MacNodeId senderId, double carrierFrequency, Codeword cw, unsigned char acid,
        std::vector<BandLimit>* bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false);

    void removePendingRac(MacNodeId nodeId);
};

#endif // _LTE_LTE_SCHEDULER_ENB_UL_H_
