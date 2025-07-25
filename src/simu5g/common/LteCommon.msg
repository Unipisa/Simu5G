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
constexpr MacNodeId UE_MAX_ID    = MacNodeId(65535);

/// Cell node ID. It is numerically equal to eNodeB MAC node ID.
typedef MacNodeId MacCellId;

/// X2 node ID. It is equal to the eNodeB MAC Cell ID
typedef MacNodeId X2NodeId;

/// Omnet Node Id
typedef unsigned int OmnetId;

/// Logical Connection Identifier
typedef unsigned short LogicalCid;

/// Connection Identifier: <MacNodeId,LogicalCid>
typedef unsigned int MacCid;

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

/// Omnet Node Id
class OmnetId {
    @existingClass; // typedef unsigned int OmnetId;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(string2int64($));
    @toString(int642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<OmnetId>($.intValue()));
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
    @existingClass; // typedef unsigned int MacCid;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(string2int64($));
    @toString(int642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<int64_t>($.intValue()));
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

