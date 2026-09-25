//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

import simu5g.common.LteCommonEnum;

cplusplus {{
#include <inet/common/geometry/Geometry_m.h>
#include "simu5g/common/LteTypes.h"
}}

namespace simu5g;

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

/// Logical Connection Identifier (used in MAC layer)
class LogicalCid {
    @existingClass; // enum class LogicalCid : unsigned short;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(LogicalCid(string2int64($)));
    @toString(int642string(num($)));
    @defaultValue(LCID_NONE);
    @toValue((omnetpp::intval_t)(num($)));
    @fromValue(LogicalCid($.intValue()));
};

/// Data Radio Bearer Identifier (used in PDCP/RLC layers, maps 1:1 to LogicalCid)
class DrbId {
    @existingClass; // enum class DrbId : unsigned short;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(DrbId(string2int64($)));
    @toString(int642string(num($)));
    @defaultValue(DRBID_NONE);
    @toValue((omnetpp::intval_t)(num($)));
    @fromValue(DrbId($.intValue()));
};

/// QoS Flow Identifier (6-bit, 0-63 per 3GPP TS 24.501)
class Qfi {
    @existingClass; // enum class Qfi : uint8_t;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(Qfi(string2int64($)));
    @toString(int642string(num($)));
    @defaultValue(QFI_NONE);
    @toValue((omnetpp::intval_t)(num($)));
    @fromValue(Qfi($.intValue()));
};

/// GTP-U Tunnel Endpoint Identifier (TS 29.281)
class Teid {
    @existingClass; // enum class Teid : uint32_t;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(Teid(string2int64($)));
    @toString(int642string(num($)));
    @defaultValue(TEID_NONE);
    @toValue((omnetpp::intval_t)(num($)));
    @fromValue(Teid($.intValue()));
};

/// Logical Channel Group id (LTE: 4 groups, NR: up to 8; TS 36.321 / TS 38.321)
class Lcg {
    @existingClass; // enum class Lcg : uint8_t;
    @descriptor(false);
    @primitive;
    @castFunction(false);
    @fromString(Lcg(string2int64($)));
    @toString(int642string(num($)));
    @defaultValue(Lcg(0));
    @toValue((omnetpp::intval_t)(num($)));
    @fromValue(Lcg($.intValue()));
};

/// Connection Identifier: <MacNodeId,LogicalCid>
class MacCid {
    @existingClass; // class MacCid with separate nodeId and lcid fields
    @descriptor(false);
    @opaque;
};

/// Node + DRB ID Identifier: <MacNodeId, DrbId> for PDCP/RLC layers
class DrbKey {
    @existingClass; // class DrbKey with separate nodeId and drbId fields
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

class RbMap
{
    @existingClass;
    @opaque;
};

// GHz type for MSG compiler (actual definition is in inet/common/Units.h, included via LteTypes.h)
namespace inet;

class GHz  { @existingClass; @primitive; @fromString(inet::GHz(string2double($)));   @toString(inet::unit2string($)); @defaultValue(inet::GHz(NaN));   @toValue(cValue($.get(),"GHz"));  @fromValue(inet::GHz($.doubleValueInUnit("GHz"))); }
