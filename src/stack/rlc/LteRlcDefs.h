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

#ifndef _LTE_LTERLCDEFS_H_
#define _LTE_LTERLCDEFS_H_

#include "common/LteControlInfo.h"
#include "stack/rlc/packet/LteRlcPdu_m.h"
#include "stack/rlc/packet/LteRlcSdu_m.h"

/*!
 * LTE RLC AM Types
 */
enum LteAmType
{
    //! Data packet
    DATA = 0,
    //****** control packets ********
    //! ACK
    ACK = 1,
    //! Move Receiver Window
    MRW = 2,
    //! Move Receiver Window ACK
    MRW_ACK = 3,
//! BITMAP
//BITMAP = 4
};

/*
 * RLC AM Window descriptor
 */

struct RlcFragDesc
{
    /*!
     * Main SDU size (bytes) - the size of  the SDU to be fragmented
     */
    int sduSize_;

    /*!
     * Fragmentation Unit (bytes) - the size of fragments the SDU will be divided into
     */
    int fragUnit_;

    /*!
     * the number of fragments the SDU will be partitioned into
     *
     */
    int totalFragments_;

    /*!
     * the fragments of current SDU already added to transmission window
     */
    int fragCounter_;

    /*!
     * the first fragment SN of current SDU
     */
    int firstSn_;

    RlcFragDesc()
    {
        fragUnit_ = 0;
        resetFragmentation();
    }
    /*
     * Configures the fragmentation descriptor for working on SDU of size sduSize
     */

    void startFragmentation(unsigned int sduSize, unsigned int firstFragment)
    {
        totalFragments_ = ceil((double) sduSize / (double) fragUnit_);
        fragCounter_ = 0;
        firstSn_ = firstFragment;
    }

    /*
     * resets the fragmentation descriptor for working on SDU of size sduSize - frag unit is left untouched.
     */

    void resetFragmentation()
    {
        sduSize_ = 0;
        totalFragments_ = 0;
        fragCounter_ = 0;
        firstSn_ = 0;
    }

    /*
     * adds a fragment to created ones. if last is added, returns true
     */

    bool addFragment()
    {
        fragCounter_++;
        if (fragCounter_ >= totalFragments_)
            return true;
        else
            return false;
    }
};

struct RlcWindowDesc
{
  public:
    //! Sequence number of the first PDU in the TxWindow
    unsigned int firstSeqNum_;
    //! Sequence number of current PDU in the TxWindow
    unsigned int seqNum_;
    //! Size of the transmission window
    unsigned int windowSize_;

    RlcWindowDesc()
    {
        seqNum_ = 0;
        firstSeqNum_ = 0;
        windowSize_ = 0;
    }
};

/*!
 * Move Receiver Window command descriptor
 */
struct MrwDesc
{
    //! MRW current Sequence Number
    unsigned int mrwSeqNum_;
    //! Last MRW Sequence Number
    unsigned int lastMrw_;

    MrwDesc()
    {
        mrwSeqNum_ = 0;
        lastMrw_ = 0;
    }
};

enum RlcAmTimerType
{
    PDU_T = 0, MRW_T = 1, BUFFER_T = 2, BUFFERSTATUS_T = 3
};


/*
 * RLC Data PDU
 */

// FI field of the RLC PDU (3GPP TS 36.322)
// It specifies whether the PDU starts/ends with the first/last byte of a SDU
//  - 00: First and last chunk of the PDU are complete SDUs
//  - 01: First chunk is a complete SDU, last chunk is a fragment
//  - 10: First chunk is a fragment, last chunk is a complete SDU
//  - 11: First and last chunk of the PDU are fragments
typedef unsigned short int FramingInfo;

/*
 * RLC UM Entity
 */
struct RlcUmRxWindowDesc
{
  public:
    //! Sequence number of the first PDU in the RxWindow
    unsigned int firstSno_;
    //! Sequence number of the first PDU in the RxWindow to be reordered
    unsigned int firstSnoForReordering_; // VR(UR)
    //! Sequence number of the PDU that triggered t_reordering timer
    unsigned int reorderingSno_; // VR(UX)
    //! Sequence number of PDU following the highest sequence number in the RxWindow
    unsigned int highestReceivedSno_;    // VR(UH)
    //! Size of the reception window
    unsigned int windowSize_;

    void clear(unsigned int i=0)
    {
        firstSno_ = i;
        firstSnoForReordering_ = i;
        reorderingSno_ = i;
        highestReceivedSno_ = i;
    }

    RlcUmRxWindowDesc()
    {
        windowSize_ = 0;  // the window size must not be cleared
        clear();
    }

};

enum RlcUmTimerType
{
    REORDERING_T = 0
};

#endif
