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

#ifndef _LTE_LTEHARQBUFFERMIRRORD2D_H_
#define _LTE_LTEHARQBUFFERMIRRORD2D_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/buffer/harq_d2d/LteHarqProcessMirrorD2D.h"
#include "stack/mac/LteMacEnb.h"

namespace simu5g {

/*
 * LteHarqBufferMirrorD2D represents a "copy" of the H-ARQ TX buffers of
 * D2D transmitting endpoint. It is used to allow the eNodeB to know whether
 * some H-ARQ process needs to be scheduled for retransmissions.
 * It contains a vector of "mirror" H-ARQ processes
 */
class LteHarqBufferMirrorD2D
{
    /// number of contained H-ARQ processes
    unsigned int numProc_;

    // Max number of HARQ retransmissions
    unsigned char maxHarqRtx_;

    /// processes vector
    std::vector<LteHarqProcessMirrorD2D *> processes_;

  public:

    /**
     * Constructor.
     *
     * @param numProc number of H-ARQ processes in the buffer.
     * @param owner simple module instantiating an H-ARQ TX buffer
     * @param nodeId UE nodeId for which this buffer has been created
     */
    LteHarqBufferMirrorD2D(unsigned int numProc, unsigned char maxHarqRtx, LteMacEnb *macOwner);

    /**
     * Manages H-ARQ feedback sent to a certain H-ARQ unit and checks if
     * the corresponding process becomes empty.
     *
     * @param fbpkt received feedback packet
     */
    void receiveHarqFeedback(inet::Packet *pkt);

    LteHarqProcessMirrorD2D *getProcess(int proc) { return processes_[proc]; }
    unsigned int getProcesses() { return numProc_; }
    void markSelectedAsWaiting();

};

} //namespace

#endif

