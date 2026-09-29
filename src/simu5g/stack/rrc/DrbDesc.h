//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _DRB_DESC_H_
#define _DRB_DESC_H_

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/mac/DrbQosProfile.h"

namespace simu5g {

// ul-DataSplitThreshold value meaning "never split" (TS 38.331): the bearer stays on
// its primary path whatever the pending data volume. The default for a split bearer that
// does not state a threshold, so it behaves as a primary-path-only bearer until told to
// split. It is the maximum int64 so it is unreachable by any real data volume; the leg
// splitter still short-circuits on it, to skip weighing the legs on every PDU of a bearer
// that will never split.
constexpr int64_t SPLIT_THRESHOLD_INFINITY = std::numeric_limits<int64_t>::max();

//
// One RLC bearer of a data radio bearer: the leg that carries it in a given cell group
// (TS 38.331 RLC-BearerConfig). A bearer with one leg is an MCG or an SCG bearer; a
// bearer with both is a split bearer.
//
// A leg carries what the configuration stated about it and nothing more: it inherits its
// bearer's RLC mode and overrides it only if its own entry says so, and the fields RRC
// decides for itself at establishment (the wire format and the SN space, see
// BearerManagement::materializeDrb()) stay unset unless a leg overrides those too. An
// unset field is not the same as a defaulted one -- keeping them apart is what lets
// establishment go on deciding whatever the configuration did not.
//
struct RlcBearerDesc {
    CellGroup cellGroup = MCG;              // which cell group serves this leg
    RlcMode rlcMode = UNKNOWN_RLC_MODE;  // rlc-Config: TM, UM or AM; UNKNOWN = not stated, the leg inherits the entry's rlcMode
    std::optional<bool> soFraming;          // wire format: false = LTE FI/concatenation (TS 36.322), true = NR SI/SO (TS 38.322)
    std::optional<unsigned int> snFieldLength;   // sn-FieldLength, in bits
};

inline std::ostream& operator<<(std::ostream& os, const RlcBearerDesc& leg) {
    os << cellGroupToA(leg.cellGroup);
    if (leg.rlcMode != UNKNOWN_RLC_MODE) os << " " << rlcModeToA(leg.rlcMode);
    if (leg.soFraming) os << (*leg.soFraming ? " SO" : " FI");
    if (leg.snFieldLength) os << " snBits=" << *leg.snFieldLength;
    return os;
}

//
// Configuration of a data radio bearer, owned by RRC (see ~DrbTable).
//
// Mirrors TS 38.331: the SDAP half of DRB-ToAddMod (cnAssociation -> SDAP-Config), and
// the RLC-BearerConfig of the bearer's logical channel (RLC mode, wire format, SN field
// length, logical channel identity and group).
//
// A split bearer (NR dual connectivity) is served by one RLC bearer per cell group, which
// the standard keeps in a list joined on drb-Identity. The legs field is that list, and
// the configuration can author it. Establishment does not read it yet: it still derives
// the legs a bearer gets, and describes a split bearer as one descriptor per leg, each
// keyed by the peer of that leg. Moving establishment onto the authored legs -- and with
// it the RLC-BearerConfig fields below, which the standard keeps per RLC bearer rather
// than per DRB -- is the next step.
//
struct DrbDesc {
    DrbKey key;                         // (peer node, DRB id); the peer is the UE at a gNB, the serving node at a UE

    // Which bearer architecture selects this bearer: CN_5GC = QoS flows via
    // mappedQfis (SDAP), CN_EPC = packet filters via filters (no SDAP). Authored
    // entries always state it; descriptors materialized from establishment alone
    // keep UNKNOWN_CORE_NETWORK.
    CoreNetwork coreNetwork = UNKNOWN_CORE_NETWORK;

    // SDAP-Config
    SessionType pduSessionType = IP_V4;
    std::string upperProtocol;          // INET protocol name for upper-layer dispatch (empty = derive from pduSessionType)
    std::vector<Qfi> mappedQfis;           // mappedQoS-FlowsToAdd
    bool isDefault = false;             // defaultDRB (5gc: fallback for unmapped QFIs; epc: carries traffic matching no filter)

    // The configuration asks for the bearer's packets to carry no SDAP header. Input to
    // the useSdapHeader decision below; only sound when a single QoS flow rides the
    // bearer, which SDAP verifies per packet (see NrSdap::recoveryQfi()).
    bool suppressSdapHeader = false;

    // Whether the bearer's packets carry an SDAP header (TS 38.331 sdap-HeaderDL/UL) --
    // the control plane's decision, computed by the BearerConfigurator once the UE's
    // default bearer is settled (see computeUseSdapHeader() there). SDAP applies it as
    // delivered and decides nothing itself.
    bool useSdapHeader = false;

    // Packet filters selecting this bearer (CN_EPC only; inet::PacketFilter
    // syntax: a message-name pattern, or an expression written as "expr(...)")
    std::vector<std::string> filters;

    // How a split bearer shares its PDUs between its legs, per direction (consumed by
    // ~DcPdcpLegSplitter). primaryPath is the preferred leg (used in either direction when
    // the bearer is not splitting).
    CellGroup primaryPath = MCG;

    // Uplink (TS 38.331 PDCP-Config, TS 38.323 5.2.1): at the UE, which sees both legs'
    // RLC queues, the bearer stays on primaryPath until the pending RLC data volume reaches
    // ulDataSplitThreshold, then uses either leg -- ulLegSelection decides which (an expr()
    // over the per-bearer PDU counter "packetOrdinal" returning the leg index), or, empty,
    // the least-occupied leg. INFINITY = never split.
    int64_t ulDataSplitThreshold = SPLIT_THRESHOLD_INFINITY;
    std::string ulLegSelection;

    // Downlink: at the DC master the secondary leg's RLC queue is at another node (across
    // X2) and cannot be weighed, so there is no threshold or load-balancing -- dlLegSelection
    // (same expr() form) decides each PDU's leg, or, empty, the bearer stays on primaryPath.
    // Real DL split flow control is X2-feedback-driven and is not modeled.
    std::string dlLegSelection;

    // The bearer's RLC bearers, one per cell group it is served in (TS 38.331 keeps them
    // in a list joined on drb-Identity). Empty = the configuration did not state them and
    // RRC derives the bearer's legs, as it always has. Authored, not yet consumed: the
    // legs a bearer is established with still come from BearerManagement::getNumLegs().
    std::vector<RlcBearerDesc> legs;

    // PDCP-Config headerCompression: the ROHC profiles the bearer uses, by their
    // configuration names (see RohcCompressor); empty = notUsed
    std::vector<std::string> rohcProfiles;

    // RLC-BearerConfig
    RlcMode rlcMode = UM;            // rlc-Config: TM, UM or AM
    bool soFraming = false;             // wire format: false = LTE FI/concatenation (TS 36.322), true = NR SI/SO (TS 38.322)
    unsigned int snFieldLength = 12;    // sn-FieldLength, in bits
    LogicalCid lcid = LCID_NONE;        // logicalChannelIdentity
    Lcg lcg = Lcg(0);                       // logicalChannelGroup, mac-LogicalChannelConfig

    // QoS profile of the bearer's flows (5QI characteristics), for QoS-aware MAC
    // scheduling; only meaningful when hasQosProfile is set (an authored entry
    // carried at least one qos field)
    bool hasQosProfile = false;
    DrbQosProfile qos;

    DrbId getDrbId() const { return key.getDrbId(); }
    MacNodeId getPeerId() const { return key.getNodeId(); }
};

inline std::ostream& operator<<(std::ostream& os, const DrbDesc& drb) {
    os << "drbId=" << drb.getDrbId() << " peer=" << drb.getPeerId();
    if (drb.coreNetwork != UNKNOWN_CORE_NETWORK) os << " " << coreNetworkToA(drb.coreNetwork);
    if (drb.isDefault) os << " DEFAULT";
    if (drb.suppressSdapHeader) os << " noSdapHeader";
    os << " qfi=[";
    for (size_t i = 0; i < drb.mappedQfis.size(); i++) {
        if (i) os << ",";
        os << drb.mappedQfis[i];
    }
    os << "]";
    if (!drb.filters.empty()) {
        os << " filters=[";
        for (size_t i = 0; i < drb.filters.size(); i++) {
            if (i) os << ",";
            os << "\"" << drb.filters[i] << "\"";
        }
        os << "]";
    }
    os << " pduSession=" << sessionTypeToA(drb.pduSessionType);
    if (!drb.rohcProfiles.empty()) {
        os << " rohc=[";
        for (size_t i = 0; i < drb.rohcProfiles.size(); i++) {
            if (i) os << ",";
            os << drb.rohcProfiles[i];
        }
        os << "]";
    }
    if (!drb.upperProtocol.empty())
        os << " upperProto=" << drb.upperProtocol;
    if (!drb.legs.empty()) {
        os << " legs=[";
        for (size_t i = 0; i < drb.legs.size(); i++) {
            if (i) os << ",";
            os << drb.legs[i];
        }
        os << "]";
        os << " primaryPath=" << cellGroupToA(drb.primaryPath);
        if (drb.ulDataSplitThreshold == SPLIT_THRESHOLD_INFINITY)
            os << " ulThreshold=infinity";
        else
            os << " ulThreshold=" << drb.ulDataSplitThreshold;
        if (!drb.ulLegSelection.empty())
            os << " ulLegSelection=" << drb.ulLegSelection;
        if (!drb.dlLegSelection.empty())
            os << " dlLegSelection=" << drb.dlLegSelection;
    }
    os << " rlc=" << rlcModeToA(drb.rlcMode) << (drb.soFraming ? " SO" : " FI")
       << " snBits=" << drb.snFieldLength
       << " lcid=" << num(drb.lcid) << " lcg=" << drb.lcg;
    if (drb.hasQosProfile)
        os << " qos: " << drb.qos;
    return os;
}

} // namespace simu5g

#endif
