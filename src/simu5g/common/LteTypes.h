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

//
// This file contains fundamental LTE type definitions that are used both
// in MSG files (via cplusplus {{ #include }}) and in C++ code.
//

#ifndef _LTE_LTETYPES_H_
#define _LTE_LTETYPES_H_

#include <map>
#include <ostream>
#include <string>

#include <omnetpp.h>
#include <inet/common/Units.h>

#include "simu5g/common/LteCommonEnum_m.h"

using namespace omnetpp;

namespace simu5g {

/**
 * Macro to define a "strong typedef" using enum class.
 * This creates a type-safe wrapper around an underlying type that:
 * - Cannot be implicitly converted to/from the underlying type
 * - Provides explicit num() function to get the underlying value
 * - Provides stream output operator
 * - Provides parsim packing/unpacking for OMNeT++ fingerprints
 *
 * Usage: SIMU5G_STRONG_TYPEDEF(NewType, unsigned short)
 */
#define SIMU5G_STRONG_TYPEDEF(TypeName, UnderlyingType) \
    enum class TypeName : UnderlyingType {}; \
    inline UnderlyingType num(TypeName id) { return static_cast<UnderlyingType>(id); } \
    inline std::ostream& operator<<(std::ostream& os, TypeName id) { os << static_cast<UnderlyingType>(id); return os; } \
    inline void doParsimPacking(omnetpp::cCommBuffer *buffer, TypeName d) { buffer->pack(num(d)); } \
    inline void doParsimUnpacking(omnetpp::cCommBuffer *buffer, TypeName& d) { UnderlyingType tmp; buffer->unpack(tmp); d = TypeName(tmp); }

/// MAC node ID
SIMU5G_STRONG_TYPEDEF(MacNodeId, unsigned short)

/// Node Id bounds  --TODO the MAX bounds are completely messed up
constexpr MacNodeId NODEID_NONE  = MacNodeId(0);
constexpr unsigned short ENB_MIN_ID   = 1;
constexpr unsigned short ENB_MAX_ID   = 1023;
constexpr unsigned short BGUE_ID      = 1024;
constexpr unsigned short UE_MIN_ID    = 1025;
constexpr unsigned short NR_UE_MIN_ID = 2049;
constexpr unsigned short BGUE_MIN_ID  = 4097;
constexpr unsigned short UE_MAX_ID    = 32767;
constexpr unsigned short MULTICAST_DEST_MIN_ID = 32768;


/// Cell node ID. It is numerically equal to eNodeB MAC node ID.
typedef MacNodeId MacCellId;

/// X2 node ID. It is equal to the eNodeB MAC Cell ID
typedef MacNodeId X2NodeId;

/// Logical Connection Identifier (used in MAC layer)
SIMU5G_STRONG_TYPEDEF(LogicalCid, unsigned short)

/// Data Radio Bearer Identifier (used in PDCP/RLC layers, maps 1:1 to LogicalCid)
SIMU5G_STRONG_TYPEDEF(DrbId, unsigned short)

/// Invalid/uninitialized LCID and DRB ID values
constexpr LogicalCid LCID_NONE = LogicalCid(65535);
constexpr DrbId DRBID_NONE = DrbId(65535);

/// Special LogicalCid values for Buffer Status Reports
// TODO add LONG/TRUNCATED BSR
// TODO FIXME these conflict with DRB LCIDs (1..), and also 0 is reserved for CCCH (common control channel)
constexpr LogicalCid SHORT_BSR = LogicalCid(0); // should be 62 (NR)
constexpr LogicalCid D2D_SHORT_BSR = LogicalCid(1); // Simu5G-specific
constexpr LogicalCid D2D_MULTI_SHORT_BSR = LogicalCid(2); // Simu5G-specific

/// Connection Identifier: <MacNodeId,LogicalCid>
// MacCid is now a class with separate fields instead of a packed integer

/// Rank Indicator
typedef unsigned short Rank;

/// Channel Quality Indicator
typedef unsigned short Cqi;


/// Transport Block Size
typedef unsigned short Tbs;

/// Logical band
typedef unsigned short Band;

/**
 *  Block allocation Map: # of Rbs per Band, per Remote.
 */
typedef std::map<Remote, std::map<Band, unsigned int>> RbMap;

/**
 * MAC Connection Identifier class that contains separate fields for
 * MacNodeId and LogicalCid instead of packing them into a single integer.
 * Can be used as std::map key and provides string representation.
 */
class MacCid
{
private:
    MacNodeId nodeId_;
    LogicalCid lcid_;

public:
    // Default constructor
    MacCid() : nodeId_(NODEID_NONE), lcid_(LCID_NONE) {}

    // Constructor
    MacCid(MacNodeId nodeId, LogicalCid lcid) : nodeId_(nodeId), lcid_(lcid) {}

    // Getters
    MacNodeId getNodeId() const { return nodeId_; }
    LogicalCid getLcid() const { return lcid_; }
    unsigned int asPackedInt() const { return (num(nodeId_) << 16) | num(lcid_); }

    // Check if this is an empty/invalid MacCid (default constructed)
    bool isEmpty() const { return nodeId_ == NODEID_NONE && lcid_ == LCID_NONE; }

    // String representation
    std::string str() const { return "MacCid(nodeId=" + std::to_string(num(nodeId_)) + ", lcid=" + std::to_string(num(lcid_)) + ")"; }

    // Comparison operators for std::map compatibility
    bool operator<(const MacCid& other) const { return asPackedInt() < other.asPackedInt(); }
    bool operator==(const MacCid& other) const  { return nodeId_ == other.nodeId_ && lcid_ == other.lcid_; }
    bool operator!=(const MacCid& other) const { return nodeId_ != other.nodeId_ || lcid_ != other.lcid_; }

    // Stream output operator
    friend std::ostream& operator<<(std::ostream& os, const MacCid& cid) { return os << cid.str(); }
};

/**
 * Node + DRB ID Identifier class for PDCP/RLC layers.
 * Semantically equivalent to MacCid but uses DrbId terminology.
 * Can be used as std::map key and provides string representation.
 */
class DrbKey
{
private:
    MacNodeId nodeId_;
    DrbId drbId_;

public:
    // Default constructor
    DrbKey() : nodeId_(NODEID_NONE), drbId_(DRBID_NONE) {}

    // Constructor
    DrbKey(MacNodeId nodeId, DrbId drbId) : nodeId_(nodeId), drbId_(drbId) {}

    // Getters
    MacNodeId getNodeId() const { return nodeId_; }
    DrbId getDrbId() const { return drbId_; }
    unsigned int asPackedInt() const { return (num(nodeId_) << 16) | num(drbId_); }

    // Check if this is an empty/invalid DrbKey (default constructed)
    bool isEmpty() const { return nodeId_ == NODEID_NONE && drbId_ == DRBID_NONE; }

    // String representation
    std::string str() const { return "DrbKey(nodeId=" + std::to_string(num(nodeId_)) + ", drbId=" + std::to_string(num(drbId_)) + ")"; }

    // Comparison operators for std::map compatibility
    bool operator<(const DrbKey& other) const { return asPackedInt() < other.asPackedInt(); }
    bool operator==(const DrbKey& other) const  { return nodeId_ == other.nodeId_ && drbId_ == other.drbId_; }
    bool operator!=(const DrbKey& other) const { return nodeId_ != other.nodeId_ || drbId_ != other.drbId_; }

    // Stream output operator
    friend std::ostream& operator<<(std::ostream& os, const DrbKey& id) { return os << id.str(); }
};

} // namespace simu5g

#endif
