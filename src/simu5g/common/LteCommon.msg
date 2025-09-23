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

import inet.common.geometry.Geometry;

import simu5g.common.LteCommonEnum;

namespace simu5g;

cplusplus {{

using namespace omnetpp;

/// MAC node ID
enum class MacNodeId : unsigned short {};  // emulate "strong typedef" with enum class

// Facilitates finding places where the numeric value of MacNodeId is used
inline unsigned short num(MacNodeId id) { return static_cast<unsigned short>(id); }

inline std::ostream& operator<<(std::ostream& os, MacNodeId id) { os << static_cast<unsigned short>(id); return os; }

// The following operators are mostly for simplifying comparisons and transformations involving UE_MIN_ID and similar constants
inline bool operator<(MacNodeId a, MacNodeId b) { return static_cast<unsigned short>(a) < static_cast<unsigned short>(b); }
inline bool operator>(MacNodeId a, MacNodeId b) { return static_cast<unsigned short>(a) > static_cast<unsigned short>(b); }
inline bool operator<=(MacNodeId a, MacNodeId b) { return static_cast<unsigned short>(a) <= static_cast<unsigned short>(b); }
inline bool operator>=(MacNodeId a, MacNodeId b) { return static_cast<unsigned short>(a) >= static_cast<unsigned short>(b); }
inline MacNodeId operator+(MacNodeId a, unsigned int b) { return MacNodeId(static_cast<unsigned short>(a) + b); }
inline MacNodeId operator-(MacNodeId a, unsigned int b) { return MacNodeId(static_cast<unsigned short>(a) - b); }
inline MacNodeId operator-(unsigned int a, MacNodeId b) { return MacNodeId(a - static_cast<unsigned short>(b)); }
inline unsigned short operator-(MacNodeId a, MacNodeId b) { return static_cast<unsigned short>(a) - static_cast<unsigned short>(b); }

// parsimPack() needed fpr the "d" fingerprint ingredient
inline void doParsimPacking(omnetpp::cCommBuffer *buffer, MacNodeId d) {buffer->pack(num(d));}
inline void doParsimUnpacking(omnetpp::cCommBuffer *buffer, MacNodeId& d) {unsigned short tmp; buffer->unpack(tmp); d = MacNodeId(tmp);}

/// Node Id bounds
constexpr MacNodeId NODEID_NONE  = MacNodeId(0);
constexpr MacNodeId ENB_MIN_ID   = MacNodeId(1);
constexpr MacNodeId ENB_MAX_ID   = MacNodeId(1023);
constexpr MacNodeId BGUE_ID      = MacNodeId(1024);
constexpr MacNodeId UE_MIN_ID    = MacNodeId(1025);
constexpr MacNodeId NR_UE_MIN_ID = MacNodeId(2049);
constexpr MacNodeId BGUE_MIN_ID  = MacNodeId(4097);
constexpr MacNodeId UE_MAX_ID    = MacNodeId(32767);

/// Cell node ID. It is numerically equal to eNodeB MAC node ID.
typedef MacNodeId MacCellId;

/// X2 node ID. It is equal to the eNodeB MAC Cell ID
typedef MacNodeId X2NodeId;

/// Logical Connection Identifier
typedef unsigned short LogicalCid;

/// Connection Identifier: <MacNodeId,LogicalCid>
// MacCid is now a class with separate fields instead of a packed integer

/// Rank Indicator
typedef unsigned short Rank;

/// Channel Quality Indicator
typedef unsigned short Cqi;

/// Precoding Matrix Index
typedef unsigned short Pmi;

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

}}

class MacCellId {
    @existingClass; // typedef MacNodeId MacCellId;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(string2int64($));
    @toString(int642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<int64_t>($.intValue()));
};

class MacNodeId {
    @existingClass; // enum class MacNodeId;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromValue(MacNodeId($.intValue()));
    @toValue(num($));
    @fromString(MacNodeId(string2int64($)));
    @toString(int642string(num($)));
}

/// X2 node ID. It is equal to the eNodeB MAC Cell ID
class X2NodeId {
    @existingClass; // typedef MacNodeId X2NodeId;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(string2int64($));
    @toString(int642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<int64_t>($.intValue()));
};

/// Logical Connection Identifier
class LogicalCid {
    @existingClass; // typedef unsigned short LogicalCid;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(string2int64($));
    @toString(int642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<LogicalCid>($.intValue()));
};

/// Connection Identifier: <MacNodeId,LogicalCid>
class MacCid {
    @existingClass; // class MacCid with separate nodeId and lcid fields
    @descriptor(false);
    @opaque;
};

/// Rank Indicator
class Rank {
    @existingClass; // typedef unsigned short Rank;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(string2int64($));
    @toString(int642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<int64_t>($.intValue()));
};

/// Channel Quality Indicator
class Cqi {
    @existingClass; // typedef unsigned short Cqi;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(string2int64($));
    @toString(int642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<int64_t>($.intValue()));
};

/// Precoding Matrix Index
class Pmi {
    @existingClass; // typedef unsigned short Pmi;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(string2int64($));
    @toString(int642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<int64_t>($.intValue()));
};

/// Transport Block Size
class Tbs {
    @existingClass; // typedef unsigned short Tbs;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(string2int64($));
    @toString(int642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<int64_t>($.intValue()));
};

/// Logical band
class Band {
    @existingClass; // typedef unsigned short Band;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(string2int64($));
    @toString(int642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<int64_t>($.intValue()));
};

struct RemoteUnitPhyData
{
    int txPower;
    inet::Coord m;
}

struct FeedbackRequest
{
    bool request;
    FeedbackGeneratorType genType;
    FeedbackType type;
    //used if genType==real
    TxMode txMode;
    bool dasAware;
    RbAllocationType rbAllocationType;
};

// no GHz in inet -- define it
cplusplus{{
#include <inet/common/Units.h>
}}

namespace inet;

cplusplus{{
using namespace units::values;
}}

class GHz  { @existingClass; @primitive; @fromString(inet::GHz(string2double($)));   @toString(inet::unit2string($)); @defaultValue(inet::GHz(NaN));   @toValue(cValue($.get(),"GHz"));  @fromValue(inet::GHz($.doubleValueInUnit("GHz"))); }
