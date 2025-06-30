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

#ifndef _LTE_LTE_SCHEDULER_ENB_DL_H_
#define _LTE_LTE_SCHEDULER_ENB_DL_H_

#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "common/LteCommon.h"
#include "stack/mac/amc/LteAmc.h"
#include "stack/mac/amc/UserTxParams.h"

namespace simu5g {

/**
 * @class LteSchedulerEnbDl
 *
 * LTE eNB downlink scheduler.
 */
class LteSchedulerEnbDl : public LteSchedulerEnb
{
    // XXX debug: to call grant from MAC
    friend class LteMacEnb;

  protected:

    //---------------------------------------------

    /**
     * Checks HARQ descriptors and returns the first free codeword.
     *
     * @param id
     * @param cw
     * @param carrierFreq
     * @return
     */
    bool checkEligibility(MacNodeId id, Codeword& cw, double carrierFrequency) override;

    bool racschedule(double carrierFrequency, BandLimitVector *bandLim = nullptr) override { return false; }

    /**
     * Updates current schedule list with HARQ retransmissions.
     * @return true if OFDM space is exhausted.
     */
    bool rtxschedule(double carrierFrequency, BandLimitVector *bandLim = nullptr) override;

    /**
     * Schedule retransmissions for background UEs
     * @return true if OFDM space is exhausted.
     */
    bool rtxscheduleBackground(double carrierFrequency, BandLimitVector *bandLim = nullptr) override;

    /**
     * Schedules retransmission for the HARQ process of the given UE on a set of logical bands.
     * Each band has also assigned a band limit amount of bytes: no more than the specified
     * amount will be served on the given band for the ACID.
     *
     * @param nodeId The node ID
     * @param cw The codeword used to serve the acid process
     * @param bands A vector of logical bands
     * @param acid The ACID
     * @return The allocated bytes. 0 if retransmission was not possible
     */
    unsigned int schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
            std::vector<BandLimit> *bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false) override;

    unsigned int scheduleBgRtx(MacNodeId bgUeId, double carrierFrequency, Codeword cw, std::vector<BandLimit> *bandLim = nullptr,
            Remote antenna = MACRO, bool limitBl = false) override;

    bool getBandLimit(std::vector<BandLimit> *bandLimit, MacNodeId ueId);

};

} //namespace

#endif // _LTE_LTE_SCHEDULER_ENB_DL_H_

