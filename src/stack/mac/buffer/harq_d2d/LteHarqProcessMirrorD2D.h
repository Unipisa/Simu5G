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

#ifndef _LTE_LTEHARQPROCESSMIRRORD2D_H_
#define _LTE_LTEHARQPROCESSMIRRORD2D_H_

#include <omnetpp.h>
#include "common/LteCommon.h"

/*
 * LteHarqProcessMirrorD2D stores the status of one H-ARQ "mirror" process
 * It contains a vector that keeps the status of each unit (there is one unit per codeword)
 */
class LteHarqProcessMirrorD2D
{
    /// current status for each codeword
    std::vector<TxHarqPduStatus> status_;

    /// bytelength of contained pdus
    std::vector<int64_t> pduLength_;

    /// Number of (re)transmissions per unit (N.B.: values are 1,2,3,4)
    std::vector<unsigned char> transmissions_;

    // number of units
    unsigned int numUnits_;

    // max number of transmissions
    unsigned char maxTransmissions_;

  public:

    LteHarqProcessMirrorD2D(unsigned int numUnits, unsigned char numTransmissions);
    void storeFeedback(HarqAcknowledgment harqAck, int64_t pduLength, Codeword cw);
    std::vector<TxHarqPduStatus>& getProcessStatus() { return status_; }
    TxHarqPduStatus getUnitStatus(Codeword cw) { return status_[cw]; }
    void markSelected(Codeword cw) { status_[cw] = TXHARQ_PDU_SELECTED; }
    void markWaiting(Codeword cw) { status_[cw] = TXHARQ_PDU_WAITING; }
    int64_t getPduLength(Codeword cw) { return pduLength_[cw]; }
    virtual ~LteHarqProcessMirrorD2D();
};

#endif
