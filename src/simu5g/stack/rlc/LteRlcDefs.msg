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

import inet.common.TagBase;
import simu5g.common.LteControlInfo;
import simu5g.stack.rlc.packet.LteRlcPdu;
import simu5g.stack.rlc.packet.LteRlcSdu;

cplusplus {{
#include "simu5g/common/LteControlInfo.h"
#include <vector>
#include <cstdint>

namespace simu5g {

/**
 * @brief Information for a NACK in a STATUS PDU, including optional byte segments and ranges.
 */
struct NackInfo {
    uint32_t sn;
    uint32_t soStart = 0;
    uint32_t soEnd = 0;
    uint32_t nackRange = 1; // Number of consecutive missing SNs
    bool isSegment = false; // True if it's a specific byte segment NACK
};

/**
 * @brief Compiled data for constructing a STATUS PDU.
 */
struct StatusPduData {
    uint32_t ackSn;
    std::vector<NackInfo> nacks;
};

} // namespace simu5g
}}

namespace simu5g;

//
// LTE RLC AM Types
//
enum LteAmType
{
    // Data packet
    DATA    = 0;
    // ****** control packets ********
    // ACK
    ACK     = 1;
    // Move Receiver Window
    MRW     = 2;
    // Move Receiver Window ACK
    MRW_ACK = 3;
    // BITMAP
    // BITMAP = 4;
};

//
// RLC AM Window descriptor
//

struct RlcFragDesc
{
    // Main SDU size (bytes) - the size of the SDU to be fragmented
    int sduSize_;

    // Fragmentation Unit (bytes) - the size of fragments the SDU will be divided into
    int fragUnit_ = 0;

    // the number of fragments the SDU will be partitioned into
    int totalFragments_;

    // the fragments of the current SDU already added to the transmission window
    int fragCounter_;

    // the first fragment SN of the current SDU
    int firstSn_;
};

cplusplus(RlcFragDesc) {{
  public:
    /*
     * Configures the fragmentation descriptor for working on an SDU of size sduSize
     */
    void startFragmentation(unsigned int sduSize, unsigned int firstFragment)
    {
        totalFragments_ = ceil((double)sduSize / (double)fragUnit_);
        fragCounter_ = 0;
        firstSn_ = firstFragment;
    }

    /*
     * resets the fragmentation descriptor for working on an SDU of size sduSize - frag unit is left untouched.
     */
    void resetFragmentation()
    {
        sduSize_ = 0;
        totalFragments_ = 0;
        fragCounter_ = 0;
        firstSn_ = 0;
    }

    /*
     * adds a fragment to the created ones. if the last is added, returns true
     */
    bool addFragment()
    {
        fragCounter_++;
        return fragCounter_ >= totalFragments_;
    }
}}

struct RlcWindowDesc
{
    //! Sequence number of the first PDU in the TxWindow
    unsigned int firstSeqNum_ = 0;
    //! Sequence number of the current PDU in the TxWindow
    unsigned int seqNum_ = 0;
    //! Size of the transmission window
    unsigned int windowSize_ = 0;
};

//
// Move Receiver Window command descriptor
//
struct MrwDesc
{
    //! MRW current Sequence Number
    unsigned int mrwSeqNum_ = 0;
    //! Last MRW Sequence Number
    unsigned int lastMrw_ = 0;
};

enum RlcAmTimerType
{
    PDU_T = 0;
    MRW_T = 1;
    BUFFER_T = 2;
    BUFFERSTATUS_T = 3;
};

//
// RLC Data PDU
//

// FI field of the RLC PDU (3GPP TS 36.322)
// It specifies whether the PDU starts/ends with the first/last byte of an SDU
//  - 00: First and last chunk of the PDU are complete SDUs
//  - 01: First chunk is a complete SDU, last chunk is a fragment
//  - 10: First chunk is a fragment, last chunk is a complete SDU
//  - 11: First and last chunk of the PDU are fragments
struct FramingInfo {
    bool firstIsFragment = false; // 1x
    bool lastIsFragment = false;  // x1
};

cplusplus(FramingInfo) {{
    short toValue() const { return (firstIsFragment ? 2 : 0) | (lastIsFragment ? 1 : 0); }
}}

//
// RLC UM Entity
//
struct RlcUmRxWindowDesc
{
    //! Sequence number of the first PDU in the RxWindow
    unsigned int firstSno_ = 0;
    //! Sequence number of the first PDU in the RxWindow to be reordered
    unsigned int firstSnoForReordering_ = 0; // VR(UR)
    //! Sequence number of the PDU that triggered t_reordering timer
    unsigned int reorderingSno_ = 0; // VR(UX)
    //! Sequence number of PDU following the highest sequence number in the RxWindow
    unsigned int highestReceivedSno_ = 0;    // VR(UH)
    //! Size of the reception window
    unsigned int windowSize_ = 0;
};

cplusplus(RlcUmRxWindowDesc) {{
    void clear(unsigned int i = 0)
    {
        firstSno_ = i;
        firstSnoForReordering_ = i;
        reorderingSno_ = i;
        highestReceivedSno_ = i;
    }
}}

enum RlcUmTimerType
{
    REORDERING_T = 0;
};

struct NackInfo { @existingClass; }
struct StatusPduData { @existingClass; }

//This should go to common anyway
class RadioLinkFailure extends inet::TagBase {
}
