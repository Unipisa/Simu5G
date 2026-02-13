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

/// MAC node ID
enum class MacNodeId : unsigned short {};  // emulate "strong typedef" with enum class

// Facilitates finding places where the numeric value of MacNodeId is used
inline unsigned short num(MacNodeId id) { return static_cast<unsigned short>(id); }

inline std::ostream& operator<<(std::ostream& os, MacNodeId id) { os << static_cast<unsigned short>(id); return os; }


// parsimPack() needed fpr the "d" fingerprint ingredient
inline void doParsimPacking(omnetpp::cCommBuffer *buffer, MacNodeId d) {buffer->pack(num(d));}
inline void doParsimUnpacking(omnetpp::cCommBuffer *buffer, MacNodeId& d) {unsigned short tmp; buffer->unpack(tmp); d = MacNodeId(tmp);}

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
typedef unsigned short LogicalCid;

/// Data Radio Bearer Identifier (used in PDCP/RLC layers, maps 1:1 to LogicalCid)
typedef LogicalCid DrbId;

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
    MacCid() : nodeId_(static_cast<MacNodeId>(0)), lcid_(0) {}

    // Constructor
    MacCid(MacNodeId nodeId, LogicalCid lcid) : nodeId_(nodeId), lcid_(lcid) {}

    // Getters
    MacNodeId getNodeId() const { return nodeId_; }
    LogicalCid getLcid() const { return lcid_; }
    unsigned int asPackedInt() const { return (num(nodeId_) << 16) | lcid_; }

    // Check if this is an empty/invalid MacCid (default constructed)
    bool isEmpty() const { return num(nodeId_) == 0 && lcid_ == 0; }

    // String representation
    std::string str() const { return "MacCid(nodeId=" + std::to_string(num(nodeId_)) + ", lcid=" + std::to_string(lcid_) + ")"; }

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
    DrbKey() : nodeId_(static_cast<MacNodeId>(0)), drbId_(0) {}

    // Constructor
    DrbKey(MacNodeId nodeId, DrbId drbId) : nodeId_(nodeId), drbId_(drbId) {}

    // Getters
    MacNodeId getNodeId() const { return nodeId_; }
    DrbId getDrbId() const { return drbId_; }
    unsigned int asPackedInt() const { return (num(nodeId_) << 16) | drbId_; }

    // Check if this is an empty/invalid DrbKey (default constructed)
    bool isEmpty() const { return num(nodeId_) == 0 && drbId_ == 0; }

    // String representation
    std::string str() const { return "DrbKey(nodeId=" + std::to_string(num(nodeId_)) + ", drbId=" + std::to_string(drbId_) + ")"; }

    // Comparison operators for std::map compatibility
    bool operator<(const DrbKey& other) const { return asPackedInt() < other.asPackedInt(); }
    bool operator==(const DrbKey& other) const  { return nodeId_ == other.nodeId_ && drbId_ == other.drbId_; }
    bool operator!=(const DrbKey& other) const { return nodeId_ != other.nodeId_ || drbId_ != other.drbId_; }

    // Stream output operator
    friend std::ostream& operator<<(std::ostream& os, const DrbKey& id) { return os << id.str(); }
};

} // namespace simu5g

#endif
