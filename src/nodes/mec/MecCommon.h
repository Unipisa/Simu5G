#ifndef  _MECCOMMON_H_
#define _MECCOMMON_H_

#include <string>
#include "common/LteCommon.h"
#include "inet/networklayer/common/L3Address.h"



class ResourceManager;

enum RlcBurstStatus
{
    START, STOP, ACTIVE, INACTIVE
};

typedef struct
{
    unsigned int pktCount = 0;
    omnetpp::simtime_t time = 0;
} Delay;

typedef struct
{
    unsigned int pktSizeCount = 0;
    omnetpp::simtime_t time = 0;
} Throughput;

typedef struct {
    unsigned int discarded = 0;
    unsigned int total = 0;
} DiscardedPkts;

typedef struct {
    uint64_t ulBits = 0;
    uint64_t dlBits = 0;
} DataVolume;

typedef struct {
    inet::L3Address addr;
    int port;
    std::string str() const { return addr.str() + ":" + std::to_string(port);}
} SockAddr;

typedef struct {
    double ram;
    double disk;
    double cpu;
} ResourceDescriptor;



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
    
    ResourceManager* getResourceManager();

}

#endif
