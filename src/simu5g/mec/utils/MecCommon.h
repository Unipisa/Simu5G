//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef  _MECCOMMON_H_
#define _MECCOMMON_H_

#include <ostream>
#include <string>

#include <inet/networklayer/common/L3Address.h>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

/*
 * Header file containing some structs used to manage MEC-related information
 */

class ResourceManager;

enum RlcBurstStatus
{
    START, STOP, ACTIVE, INACTIVE
};

struct Delay {
    unsigned int pktCount = 0;
    simtime_t time = 0;
};

inline std::ostream& operator<<(std::ostream& os, const Delay& e)
{
    os << "pktCount=" << e.pktCount << " time=" << e.time;
    return os;
}

struct Throughput {
    unsigned int pktSizeCount = 0;
    simtime_t time = 0;
};

inline std::ostream& operator<<(std::ostream& os, const Throughput& e)
{
    os << "pktSizeCount=" << e.pktSizeCount << " time=" << e.time;
    return os;
}

struct DiscardedPkts {
    unsigned int discarded = 0;
    unsigned int total = 0;
};

inline std::ostream& operator<<(std::ostream& os, const DiscardedPkts& e)
{
    os << "discarded=" << e.discarded << " total=" << e.total;
    return os;
}

struct DataVolume {
    uint64_t ulBits = 0;
    uint64_t dlBits = 0;
};

inline std::ostream& operator<<(std::ostream& os, const DataVolume& e)
{
    os << "ulBits=" << e.ulBits << " dlBits=" << e.dlBits;
    return os;
}

struct SockAddr {
    inet::L3Address addr;
    int port;
    std::string str() const { return addr.str() + ":" + std::to_string(port); }
};

/*
 * Structure representing some useful information of the MEC service ServiceInfo
 * attribute according to the ETSI standard:
 * ETSI GS MEC 011 V2.2.1 (2020-12) - 8.1.2.2 Type: ServiceInfo
 */
struct ServiceDescriptor {
    std::string mecHostname;
    std::string name;
    std::string version;
    std::string serialize;

    std::string transportId;
    std::string transportName;
    std::string transportType;
    std::string transportProtocol;

    std::string catHref;
    std::string catId;
    std::string catName;
    std::string catVersion;

    std::string scopeOfLocality;
    bool isConsumedLocallyOnly;

    inet::L3Address addr;
    int port;
    std::string str() const { return addr.str() + ":" + std::to_string(port); }
};

struct ResourceDescriptor {
    double ram;
    double disk;
    double cpu;
};

inline std::ostream& operator<<(std::ostream& os, const ResourceDescriptor& e)
{
    os << "ram=" << e.ram << " disk=" << e.disk << " cpu=" << e.cpu;
    return os;
}

namespace mec {

/*
 * associated identifier for a UE
 *
 * type:
 *      0 = reserved.
 *      1 = UE_IPV4_ADDRESS.
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

inline std::ostream& operator<<(std::ostream& os, const AssociateId& e)
{
    os << "type=" << e.type << " value=" << e.value;
    return os;
}

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

inline std::ostream& operator<<(std::ostream& os, const Plmn& e)
{
    os << "mcc=" << e.mcc << " mnc=" << e.mnc;
    return os;
}

/*
 * E-UTRAN Cell Global Identifier as defined in ETSI TS 136 413
 *
 *  plmn: Public Land Mobile Network Identity.
 *  cellid: E-UTRAN Cell Global Identifier (CellId in our case)
 *
 */
struct Ecgi
{
    Plmn plmn;
    MacCellId cellId;
};

inline std::ostream& operator<<(std::ostream& os, const Ecgi& e)
{
    os << "plmn=[" << e.plmn << "] cellId=" << e.cellId;
    return os;
}

struct Timestamp
{
    int seconds;
    int nanoSeconds;
};

} // namespace mec

} //namespace

#endif

