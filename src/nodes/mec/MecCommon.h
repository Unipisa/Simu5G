#ifndef  _MECCOMMON_H_
#define _MECCOMMON_H_

#include <string>
#include "common/LteCommon.h"


enum RlcBurstStatus
{
    START, STOP, ACTIVE, INACTIVE
};

enum Trigger
{
    L2_MEAS_PERIODICAL, L2_MEAS_UTI_80, L2_MEAS_DL_TPU_1, UE_CQI
};


/*
 * temporary method to get the trigger of a RNI notification.
 * unused since the subscription are not implemented, yet
 */
Trigger getTrigger(std::string& trigger);

typedef struct
{
    unsigned int pktCount;
    omnetpp::simtime_t time;
} Delay;

typedef struct
{
    unsigned int pktSizeCount;
    omnetpp::simtime_t time;
} Throughput;

typedef struct {
    unsigned int discarded;
    unsigned int total;
} DiscardedPkts;

typedef struct {
    uint64_t ulBits;
    uint64_t dlBits;
} DataVolume;


namespace mec {

    /*
     * associated identifier for a UE
     *
     * type:
     *      0 = reserved.
     *      1 = UE_IPv4_ADDRESS.
     *      2 = UE_IPV6_ADDRESS.
     *      3 = NATED_IP_ADDRESS.
     *      4 = GTP_TEID.
     *
     * value:
     *       Value for the identifier.
     */
    struct AssociateId 
    {
        std::string type;
        std::string value;
    };

    /*
     * Public Land Mobile Network Identity as defined in ETSI TS 136 413
     *
     * mcc: The Mobile Country Code part of PLMN Identity
     * mnc: The Mobile Network Code part of PLMN Identity
     */
    struct Plmn
    {
        std::string mcc;
        std::string mnc;
    };

    /*
     * E-UTRAN CelI Global Identifier as defined in ETSI TS 136 413
     *
     *  plmn: Public Land Mobile Network Identity.
     *  cellid: E-UTRAN CelI Global Identifier (CellId in our case)
     *
     */
    struct Ecgi
    {
        Plmn plmn;
        MacCellId cellId;
    };

    struct Timestamp
    {
        int secods;
        int nanoSecods;
        
    };
    
}

#endif
