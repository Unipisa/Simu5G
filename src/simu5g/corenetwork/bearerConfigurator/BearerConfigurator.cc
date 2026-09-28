//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <inet/common/ModuleAccess.h>
#include <inet/common/PatternMatcher.h>
#include <inet/common/stlutils.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "simu5g/common/InitStages.h"
#include "simu5g/common/QfiRuleSet.h"
#include "simu5g/corenetwork/bearerConfigurator/BearerConfigurator.h"
#include <algorithm>
#include "simu5g/corenetwork/userPlaneNodeControl/UserPlaneNodeControl.h"
#include "simu5g/stack/pdcp/rohc/RohcCompressor.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(BearerConfigurator);

typedef BearerConfigurator::AuthoredBearer AuthoredBearer;

// The complete field vocabulary of a bearer-definition entry (staticDrbs/onDemandDrbs);
// anything else in an entry or profile is rejected as a typo
static const std::vector<std::string> KNOWN_ENTRY_FIELDS = {
    "coreNetwork", "ue", "drbId", "profile", "mappedQfis", "filters", "lcg", "rlcMode",
    "legs", "primaryPath", "ulDataSplitThreshold", "ulLegSelection", "dlLegSelection",
    "pduSessionType", "upperProtocol", "isDefault", "suppressSdapHeader", "rohc",
    "gbr", "packetDelayBudget", "packetErrorRate", "qosPriorityLevel"
};

// The entry fields a profile may not carry: a profile describes what a bearer is,
// never which UE it belongs to (ue, drbId) or which architecture and flows select
// it (coreNetwork, mappedQfis, filters, isDefault, and suppressSdapHeader, whose
// validity is bound to them)
static const std::vector<std::string> FORBIDDEN_PROFILE_FIELDS = {
    "drbId", "ue", "profile", "coreNetwork", "mappedQfis", "filters", "isDefault", "suppressSdapHeader"
};

void BearerConfigurator::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        // after INITSTAGE_SIMU5G_NODE_RELATIONSHIPS, so the UEs' serving nodes are known
        configureDrbs();
        validateQfiRules();
    }
    else if (stage == inet::INITSTAGE_LAST) {
        validateStaticDrbs();
    }
}

BearerConfigurator::~BearerConfigurator()
{
    // plain delete, not dropAndDelete(): deleting while still owned lets ~cOwnedObject
    // deregister from this module's owner list (dropAndDelete nulls the owner first,
    // which leaves a stale list slot that crashes cComponent's teardown)
    delete predefinedDrbProfiles_;
}

const cValueMap *BearerConfigurator::getPredefinedDrbProfiles()
{
    if (predefinedDrbProfiles_ == nullptr) {
        // The standardized QoS characteristics tables as built-in profiles, referenced
        // from entries exactly like user-defined ones: "qci-N" carries the QCI rows of
        // TS 23.203 Table 6.1.7-A, "5qi-N" the 5QI rows of TS 23.501 Table 5.7.4-1
        // (the core 1..9 of each; row 1 = conversational voice, 2 = conversational
        // video, 3 = real-time gaming, 4 = buffered video, 5 = IMS signalling,
        // 6/8 = buffered video and TCP services, 7 = voice/live video/interactive
        // gaming, 9 = the default best-effort bearer). A row carries what the spec
        // standardizes -- resource type, priority, packet delay budget, packet error
        // rate -- and nothing else: the RLC mode and the logical channel group are RAN
        // choices, not row columns, so an entry states them itself if it needs to.
        //
        // NOTE the two tables use different priority scales (QCI 1..9, 5QI 10..90), and
        // both differ from the small hand-picked values existing configurations use; the
        // QoS-aware scheduler weights bearers by 1/(priority+1), so profiles from
        // different scales should not be mixed within one network.
        //
        // Built by evaluating the literal below, so the container ownership is exactly
        // that of an ini-file object value.
        static const char *table = R"({
            "qci-1": {gbr: true,  qosPriorityLevel: 2, packetDelayBudget: 100, packetErrorRate: 1e-2},
            "qci-2": {gbr: true,  qosPriorityLevel: 4, packetDelayBudget: 150, packetErrorRate: 1e-3},
            "qci-3": {gbr: true,  qosPriorityLevel: 3, packetDelayBudget: 50,  packetErrorRate: 1e-3},
            "qci-4": {gbr: true,  qosPriorityLevel: 5, packetDelayBudget: 300, packetErrorRate: 1e-6},
            "qci-5": {gbr: false, qosPriorityLevel: 1, packetDelayBudget: 100, packetErrorRate: 1e-6},
            "qci-6": {gbr: false, qosPriorityLevel: 6, packetDelayBudget: 300, packetErrorRate: 1e-6},
            "qci-7": {gbr: false, qosPriorityLevel: 7, packetDelayBudget: 100, packetErrorRate: 1e-3},
            "qci-8": {gbr: false, qosPriorityLevel: 8, packetDelayBudget: 300, packetErrorRate: 1e-6},
            "qci-9": {gbr: false, qosPriorityLevel: 9, packetDelayBudget: 300, packetErrorRate: 1e-6},
            "5qi-1": {gbr: true,  qosPriorityLevel: 20, packetDelayBudget: 100, packetErrorRate: 1e-2},
            "5qi-2": {gbr: true,  qosPriorityLevel: 40, packetDelayBudget: 150, packetErrorRate: 1e-3},
            "5qi-3": {gbr: true,  qosPriorityLevel: 30, packetDelayBudget: 50,  packetErrorRate: 1e-3},
            "5qi-4": {gbr: true,  qosPriorityLevel: 50, packetDelayBudget: 300, packetErrorRate: 1e-6},
            "5qi-5": {gbr: false, qosPriorityLevel: 10, packetDelayBudget: 100, packetErrorRate: 1e-6},
            "5qi-6": {gbr: false, qosPriorityLevel: 60, packetDelayBudget: 300, packetErrorRate: 1e-6},
            "5qi-7": {gbr: false, qosPriorityLevel: 70, packetDelayBudget: 100, packetErrorRate: 1e-3},
            "5qi-8": {gbr: false, qosPriorityLevel: 80, packetDelayBudget: 300, packetErrorRate: 1e-6},
            "5qi-9": {gbr: false, qosPriorityLevel: 90, packetDelayBudget: 300, packetErrorRate: 1e-6}
        })";
        cDynamicExpression expr;
        expr.parse(table);
        auto *map = check_and_cast<cValueMap *>(expr.evaluate(this).objectValue());
        take(map);
        predefinedDrbProfiles_ = map;
    }
    return predefinedDrbProfiles_;
}

void BearerConfigurator::computeUseSdapHeader(DrbDesc& drb)
{
    drb.useSdapHeader = drb.coreNetwork == CN_5GC
            && (drb.isDefault || drb.mappedQfis.size() > 1) && !drb.suppressSdapHeader;
}

void BearerConfigurator::configureDrbs()
{
    // The node ids of each registered UE: one per stack, both naming the same UE and so
    // the same bearers. The LTE id, which every UE has, serves as the UE's identity here.
    std::map<cModule *, std::vector<MacNodeId>> ueNodeIds;
    for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
        if (getNodeTypeById(nodeId) == UE && info.moduleRef != nullptr)
            ueNodeIds[info.moduleRef].push_back(nodeId);

    // UE module paths in the parameters are relative to the network
    std::string networkPrefix = std::string(getSystemModule()->getFullPath()) + ".";

    // The static bearers of each UE, collected before anything is pushed, so that the
    // default DRB of a UE can be settled while all of its bearers are in hand
    std::map<cModule *, std::map<DrbId, DrbDesc>> drbsOfUe;

    // A user profile must not redefine a predefined row: silently shadowing a
    // standardized name would make "5qi-1" mean different things in different inis.
    // Its fields must come from the entry vocabulary, minus the per-bearer ones
    // (see FORBIDDEN_PROFILE_FIELDS); anything else is a typo, and a typo'd field
    // would otherwise be silently ignored.
    if (const cValueMap *userProfiles = check_and_cast_nullable<const cValueMap *>(par("drbProfiles").objectValue()))
        for (const auto& [profileName, value] : userProfiles->getFields()) {
            if (getPredefinedDrbProfiles()->containsKey(profileName.c_str()))
                throw cRuntimeError("drbProfiles entry '%s' redefines a predefined profile; the standardized rows cannot be overridden",
                        profileName.c_str());
            const cValueMap *profile = check_and_cast<const cValueMap *>(value.objectValue());
            for (const auto& [key, fieldValue] : profile->getFields()) {
                if (contains(FORBIDDEN_PROFILE_FIELDS, key))
                    throw cRuntimeError("drbProfiles entry '%s' must not contain the '%s' field", profileName.c_str(), key.c_str());
                if (!contains(KNOWN_ENTRY_FIELDS, key))
                    throw cRuntimeError("drbProfiles entry '%s' contains unknown field '%s'", profileName.c_str(), key.c_str());
            }
        }

    // The QoS-derivation policy the definitions below may rely on
    amPerThreshold_ = par("amPerThreshold");
    lcgPriorityBounds_.clear();
    auto *boundsArr = check_and_cast<const cValueArray *>(par("lcgPriorityBounds").objectValue());
    if (boundsArr->size() != NUM_LCGS - 1)
        throw cRuntimeError("lcgPriorityBounds must have %d entries, one bound between each pair of adjacent LCGs",
                NUM_LCGS - 1);
    for (int i = 0; i < (int)boundsArr->size(); i++) {
        long bound = (long)boundsArr->get(i).intValue();
        if (i > 0 && bound <= lcgPriorityBounds_.back())
            throw cRuntimeError("lcgPriorityBounds must be strictly ascending");
        lcgPriorityBounds_.push_back(bound);
    }
    rohcForDrbProfiles_.clear();
    const cValueMap *userProfiles = check_and_cast_nullable<const cValueMap *>(par("drbProfiles").objectValue());
    auto *rohcArr = check_and_cast<const cValueArray *>(par("rohcForDrbProfiles").objectValue());
    for (int i = 0; i < (int)rohcArr->size(); i++) {
        std::string name = rohcArr->get(i).stdstringValue();
        if (!getPredefinedDrbProfiles()->containsKey(name.c_str()) && !(userProfiles && userProfiles->containsKey(name.c_str())))
            throw cRuntimeError("rohcForDrbProfiles names unknown profile '%s'", name.c_str());
        rohcForDrbProfiles_.insert(name);
    }

    parseDrbDefinitions("staticDrbs", false, ueNodeIds, networkPrefix, drbsOfUe);
    parseDrbDefinitions("onDemandDrbs", true, ueNodeIds, networkPrefix, drbsOfUe);

    // A QFI is either mapped up front by a static definition or serves as an on-demand
    // selector; both claiming it would leave the on-demand entry permanently dead
    for (const AuthoredBearer& ab : authoredBearers_) {
        if (!ab.onDemand || ab.desc.coreNetwork != CN_5GC)
            continue;
        auto uit = drbsOfUe.find(ab.ueModule);
        if (uit == drbsOfUe.end())
            continue;
        for (const auto& [drbId, staticDrb] : uit->second)
            for (Qfi qfi : ab.desc.mappedQfis)
                if (contains(staticDrb.mappedQfis, qfi))
                    throw cRuntimeError("onDemandDrbs: QFI %d of UE '%s' is already mapped to static DRB %d",
                            (int)num(qfi), ab.ueModule->getFullPath().c_str(), (int)num(drbId));
    }

    for (auto& [ueModule, drbs] : drbsOfUe) {
        // The default DRB is where traffic with no QFI-to-DRB mapping (5gc) or no
        // matching packet filter (epc) goes. If the configuration marks none in
        // either table, a UE's single static bearer takes the role; a UE with
        // several bearers must mark one explicitly -- inferring it from entry
        // order would hide configuration mistakes.
        bool onDemandDefault = false;
        for (const AuthoredBearer& ab : authoredBearers_)
            if (ab.onDemand && ab.ueModule == ueModule && ab.desc.isDefault)
                onDemandDefault = true;
        if (!onDemandDefault && std::none_of(drbs.begin(), drbs.end(), [](const auto& e) { return e.second.isDefault; })) {
            if (drbs.size() > 1)
                throw cRuntimeError("no default DRB designated for UE '%s': it has %d static bearers and no "
                        "entry of either table marks isDefault=true -- with several bearers the choice "
                        "must be explicit", ueModule->getFullPath().c_str(), (int)drbs.size());
            drbs.begin()->second.isDefault = true;
        }

        // The retained records are what establishment-time matching consults, so the
        // settled default is propagated into them
        for (AuthoredBearer& ab : authoredBearers_)
            if (!ab.onDemand && ab.ueModule == ueModule)
                ab.desc.isDefault = drbs.at(ab.desc.getDrbId()).isDefault;

    }

    // The header decision of the retained records, now that every UE's default bearer
    // is settled; an on-demand record's descriptor copy inherits it at materialization
    // (see establishFromDefinition()/drbOfDefinition())
    for (AuthoredBearer& ab : authoredBearers_)
        computeUseSdapHeader(ab.desc);
}

// Whether the UE's stack contains SDAP. Structure, not configuration: the sdap
// submodule exists iff the NIC's hasSdap is set, the same resolvability test
// BearerManagement::configureDrb() applies on its own side.
bool BearerConfigurator::ueStackHasSdap(const cModule *ueModule)
{
    const cModule *nic = ueModule->getSubmodule("cellularNic");
    return nic != nullptr && nic->getSubmodule("sdap") != nullptr;
}

// The "rohc" field of a bearer definition: true (every modeled profile), false, or an
// object with an optional "profiles" list of profile names (omitted = every modeled one).
// Returns the profiles, empty for false.
static std::vector<std::string> parseRohcField(const cValue& value, const char *paramName, int i)
{
    if (value.getType() == cValue::BOOL)
        return value.boolValue() ? rohcProfileNames() : std::vector<std::string>();
    const cValueMap *rohc = value.containsObject() ? dynamic_cast<const cValueMap *>(value.objectValue()) : nullptr;
    if (rohc == nullptr)
        throw cRuntimeError("%s entry %d: \"rohc\" must be true, false, or an object like {profiles: [\"udp\", \"ip\"]}", paramName, i);
    for (const auto& [key, fieldValue] : rohc->getFields())
        if (key != "profiles")
            throw cRuntimeError("%s entry %d: unknown field '%s' in \"rohc\" (the only one is \"profiles\")", paramName, i, key.c_str());
    if (!rohc->containsKey("profiles"))
        return rohcProfileNames();
    const cValueArray *arr = dynamic_cast<const cValueArray *>(rohc->get("profiles").containsObject() ? rohc->get("profiles").objectValue() : nullptr);
    if (arr == nullptr || arr->size() == 0)
        throw cRuntimeError("%s entry %d: \"rohc\" profiles must be a non-empty list of profile names", paramName, i);
    std::vector<std::string> profiles;
    for (int j = 0; j < (int)arr->size(); j++) {
        std::string name = arr->get(j).stdstringValue();
        if (!contains(rohcProfileNames(), name))
            throw cRuntimeError("%s entry %d: unknown or unmodeled ROHC profile \"%s\" (available: \"rtp\", \"udp\", \"tcp\", \"ip\")", paramName, i, name.c_str());
        if (contains(profiles, name))
            throw cRuntimeError("%s entry %d: duplicate ROHC profile \"%s\"", paramName, i, name.c_str());
        profiles.push_back(name);
    }
    return profiles;
}

void BearerConfigurator::parseDrbDefinitions(const char *paramName, bool onDemand,
        const std::map<cModule *, std::vector<MacNodeId>>& ueNodeIds, const std::string& networkPrefix,
        std::map<cModule *, std::map<DrbId, DrbDesc>>& drbsOfUe)
{
    const cValueArray *arr = check_and_cast_nullable<const cValueArray *>(par(paramName).objectValue());
    const cValueMap *profiles = check_and_cast_nullable<const cValueMap *>(par("drbProfiles").objectValue());
    if (arr == nullptr || arr->size() == 0)
        return;

    for (int i = 0; i < (int)arr->size(); i++) {
        const cValueMap *entry = check_and_cast<const cValueMap *>(arr->get(i).objectValue());

        // A typo'd field name would be silently ignored, so unknown fields are rejected
        for (const auto& [key, value] : entry->getFields())
            if (!contains(KNOWN_ENTRY_FIELDS, key))
                throw cRuntimeError("%s entry %d: unknown field '%s'", paramName, i, key.c_str());

        // Resolve the entry's named profile, if any (user profiles were validated
        // up front, see configureDrbs())
        const cValueMap *profile = nullptr;
        if (entry->containsKey("profile")) {
            const char *name = entry->get("profile").stringValue();
            if (profiles && profiles->containsKey(name))
                profile = check_and_cast<const cValueMap *>(profiles->get(name).objectValue());
            else if (getPredefinedDrbProfiles()->containsKey(name))
                profile = check_and_cast<const cValueMap *>(getPredefinedDrbProfiles()->get(name).objectValue());
            else {
                std::string available;
                if (profiles)
                    for (const auto& [profileName, value] : profiles->getFields())
                        available += (available.empty() ? "" : ", ") + profileName;
                throw cRuntimeError("%s entry %d references unknown profile '%s' (available: %s; plus the predefined \"qci-1\"..\"qci-9\" and \"5qi-1\"..\"5qi-9\")",
                        paramName, i, name, available.empty() ? "none" : available.c_str());
            }
        }

        // Field lookup: the entry's own value wins over the profile's
        auto field = [&](const char *key) -> const cValue * {
            if (entry->containsKey(key))
                return &entry->get(key);
            if (profile && profile->containsKey(key))
                return &profile->get(key);
            return nullptr;
        };

        // DRB id: static definitions pin it; an on-demand definition gets one assigned
        // when it first matches (see establishFromDefinition()/drbOfDefinition())
        DrbDesc drb;
        DrbId drbId = DRBID_NONE;
        drb.key = DrbKey(NODEID_NONE, DRBID_NONE);
        if (!onDemand) {
            long id = entry->get("drbId").intValue();
            if (id < 1 || id > MAX_DRB_ID)
                throw cRuntimeError("%s entry %d: invalid drbId %ld, must be 1..%d (TS 38.331 DRB-Identity)", paramName, i, id, MAX_DRB_ID);
            drbId = DrbId(id);
            drb.key = DrbKey(NODEID_NONE, drbId);
            drb.lcid = LogicalCid(num(drbId));
        }
        else if (entry->containsKey("drbId"))
            throw cRuntimeError("%s entry %d: on-demand definitions do not name a \"drbId\"; one is assigned when the entry first matches", paramName, i);

        // coreNetwork (required): which architecture selects the bearer. Stated per
        // entry, never inferred; the receiving RRC checks it against its own stack
        // (see BearerManagement::configureDrb()).
        if (!entry->containsKey("coreNetwork"))
            throw cRuntimeError("%s entry %d: missing required field \"coreNetwork\" (\"epc\" or \"5gc\")", paramName, i);
        std::string coreNetworkStr = entry->get("coreNetwork").stdstringValue();
        drb.coreNetwork = aToCoreNetwork(coreNetworkStr);
        if (drb.coreNetwork == UNKNOWN_CORE_NETWORK)
            throw cRuntimeError("%s entry %d: invalid coreNetwork '%s', must be \"epc\" or \"5gc\"", paramName, i, coreNetworkStr.c_str());

        // isDefault (optional; if no entry of a UE is marked, its first static one
        // becomes default).
        if (entry->containsKey("isDefault"))
            drb.isDefault = entry->get("isDefault").boolValue();

        // mappedQfis (5gc only, optional; an entry without it does not take part in SDAP's
        // QFI-to-DRB mapping, e.g. it only carries the bearer's QoS profile)
        if (entry->containsKey("mappedQfis")) {
            if (drb.coreNetwork != CN_5GC)
                throw cRuntimeError("%s entry %d: \"mappedQfis\" is a \"5gc\" selector, not valid on a \"%s\" bearer", paramName, i, coreNetworkStr.c_str());
            const cValueArray *qfiArr = check_and_cast<const cValueArray *>(entry->get("mappedQfis").objectValue());
            for (int j = 0; j < (int)qfiArr->size(); j++) {
                long q = qfiArr->get(j).intValue();
                if (q < 0 || q > 63)
                    throw cRuntimeError("%s entry %d: invalid QFI %ld, must be 0..63", paramName, i, q);
                if (contains(drb.mappedQfis, Qfi(q)))
                    throw cRuntimeError("%s entry %d: duplicate QFI %ld in \"mappedQfis\"", paramName, i, q);
                drb.mappedQfis.push_back(Qfi(q));
            }
        }

        // An on-demand "5gc" definition is selected by its mappedQfis, so one without
        // them could never match on a QFI -- unless it is the default, which is selected
        // by not matching anything else (it catches the QFIs no other bearer maps).
        if (onDemand && drb.coreNetwork == CN_5GC && drb.mappedQfis.empty() && !drb.isDefault)
            throw cRuntimeError("%s entry %d: an on-demand \"5gc\" definition needs a non-empty \"mappedQfis\" "
                    "unless it is the default -- it is selected by its QFIs and could never match without them",
                    paramName, i);

        // suppressSdapHeader ("5gc" only, optional): ask for the bearer's packets to
        // carry no SDAP header. Meaningful on the default bearer only -- a non-default
        // bearer with at most one mapped QFI is headerless already, and one with
        // several needs the header to tell its flows apart, which is also why a
        // suppressed default may map at most one QFI. The resulting decision is
        // computed in computeUseSdapHeader(), and SDAP verifies the single-flow
        // assertion per packet (see NrSdap::recoveryQfi()).
        if (entry->containsKey("suppressSdapHeader")) {
            if (drb.coreNetwork != CN_5GC)
                throw cRuntimeError("%s entry %d: \"suppressSdapHeader\" is a \"5gc\" field, not valid on a \"%s\" "
                        "bearer (a stack without SDAP has no SDAP header)", paramName, i, coreNetworkStr.c_str());
            drb.suppressSdapHeader = entry->get("suppressSdapHeader").boolValue();
            if (drb.suppressSdapHeader && !drb.isDefault)
                throw cRuntimeError("%s entry %d: \"suppressSdapHeader\" is only meaningful on the default bearer "
                        "(marked \"isDefault\") -- a non-default bearer with at most one mapped QFI carries no "
                        "header anyway", paramName, i);
            if (drb.suppressSdapHeader && drb.mappedQfis.size() > 1)
                throw cRuntimeError("%s entry %d: \"suppressSdapHeader\" asserts that a single QoS flow rides the "
                        "bearer, but %d QFIs are mapped to it", paramName, i, (int)drb.mappedQfis.size());
        }

        // filters (eps only, optional; the packet filters that select this bearer --
        // an entry without them can still be the default bearer or carry only a QoS
        // profile). Compiled below, once per matched UE, so a syntax error fails at
        // setup, not on the first packet.
        if (entry->containsKey("filters")) {
            if (drb.coreNetwork != CN_EPC)
                throw cRuntimeError("%s entry %d: \"filters\" is an \"epc\" selector, not valid on a \"%s\" bearer", paramName, i, coreNetworkStr.c_str());
            const cValueArray *fArr = check_and_cast<const cValueArray *>(entry->get("filters").objectValue());
            for (int j = 0; j < (int)fArr->size(); j++)
                drb.filters.push_back(fArr->get(j).stdstringValue());
        }

        // QoS profile (all optional; any of them present = the bearer has a QoS profile,
        // which RRC pushes into the eNB/gNB MAC for QoS-aware scheduling)
        drb.hasQosProfile = field("gbr") || field("packetDelayBudget") || field("packetErrorRate") || field("qosPriorityLevel");
        if (const cValue *v = field("gbr"))
            drb.qos.gbr = v->boolValue();
        if (const cValue *v = field("packetDelayBudget"))
            drb.qos.delayBudgetMs = v->doubleValue();
        if (const cValue *v = field("packetErrorRate"))
            drb.qos.packetErrorRate = v->doubleValue();
        if (const cValue *v = field("qosPriorityLevel")) {
            long p = (long)v->intValue();
            if (p < 1 || p > 127)
                throw cRuntimeError("%s entry %d: invalid qosPriorityLevel %ld, must be 1..127 "
                        "(the 3GPP priority level range; lower = more important)", paramName, i, p);
            drb.qos.priorityLevel = (int)p;
        }

        // rlcMode: stated by the definition, or derived from its QoS profile's packet
        // error rate -- a PER target HARQ alone cannot meet gets ARQ, the RAN-side
        // decision a gNB makes from the delivered QoS profile (amPerThreshold).
        // Required when there is nothing to derive it from.
        const cValue *rlcModeVal = field("rlcMode");
        if (rlcModeVal != nullptr) {
            std::string rlcModeStr = rlcModeVal->stdstringValue();
            drb.rlcMode = aToRlcMode(rlcModeStr);
            if (drb.rlcMode == TM)
                throw cRuntimeError("%s entry %d: rlcMode \"TM\" is not valid for a data radio bearer -- "
                        "3GPP transparent mode carries BCCH/PCCH/SRB0-class channels, not DRBs; "
                        "use \"UM\" or \"AM\"", paramName, i);
            if (drb.rlcMode == UNKNOWN_RLC_MODE)
                throw cRuntimeError("%s entry %d: invalid rlcMode '%s', must be \"UM\" or \"AM\"",
                        paramName, i, rlcModeStr.c_str());
        }
        else if (field("packetErrorRate") != nullptr)
            drb.rlcMode = (drb.qos.packetErrorRate <= amPerThreshold_) ? AM : UM;
        else
            throw cRuntimeError("%s entry %d: missing \"rlcMode\" -- a bearer definition must state its "
                    "RLC mode (\"UM\" or \"AM\") in the entry or its profile, or carry a QoS "
                    "profile with a \"packetErrorRate\" to derive it from", paramName, i);

        // lcg: stated by the definition, or derived from its QoS profile's priority
        // level (bucketed by lcgPriorityBounds); 0 when there is nothing to derive
        // it from
        if (const cValue *v = field("lcg")) {
            long lcgVal = (long)v->intValue();
            if (lcgVal < 0 || lcgVal >= NUM_LCGS)
                throw cRuntimeError("%s entry %d: invalid lcg %ld, must be 0..%d",
                        paramName, i, lcgVal, NUM_LCGS - 1);
            drb.lcg = Lcg(lcgVal);
        }
        else if (field("qosPriorityLevel") != nullptr) {
            int bucket = 0;
            while (bucket < (int)lcgPriorityBounds_.size() && (long)drb.qos.priorityLevel >= lcgPriorityBounds_[bucket])
                bucket++;
            drb.lcg = Lcg(bucket);
        }

        // legs (optional): the cell groups that serve this bearer, i.e. its RLC bearers.
        // An element is either a leg name, or an object naming the leg and overriding the
        // RLC fields it inherits from the entry. Omitted = the configuration does not say,
        // and RRC derives the bearer's legs as it always has.
        if (const cValue *v = field("legs")) {
            const cValueArray *legArr = check_and_cast<const cValueArray *>(v->objectValue());
            if (legArr->size() == 0)
                throw cRuntimeError("%s entry %d: \"legs\" is empty; omit it to leave the legs to RRC", paramName, i);
            for (int j = 0; j < (int)legArr->size(); j++) {
                const cValue& legValue = legArr->get(j);
                const cValueMap *legEntry = nullptr;
                std::string groupStr;
                if (legValue.getType() == cValue::OBJECT) {
                    legEntry = check_and_cast<const cValueMap *>(legValue.objectValue());
                    for (const auto& [key, value] : legEntry->getFields())
                        if (key != "leg" && key != "rlcMode" && key != "soFraming" && key != "snFieldLength")
                            throw cRuntimeError("%s entry %d, leg %d: unknown field '%s'", paramName, i, j, key.c_str());
                    if (!legEntry->containsKey("leg"))
                        throw cRuntimeError("%s entry %d, leg %d: missing required field \"leg\"", paramName, i, j);
                    groupStr = legEntry->get("leg").stdstringValue();
                }
                else
                    groupStr = legValue.stdstringValue();

                RlcBearerDesc leg;
                leg.cellGroup = aToCellGroup(groupStr);
                if (leg.cellGroup == UNKNOWN_CELL_GROUP)
                    throw cRuntimeError("%s entry %d, leg %d: invalid cell group '%s', must be \"MCG\" or \"SCG\"",
                            paramName, i, j, groupStr.c_str());
                for (const RlcBearerDesc& earlier : drb.legs)
                    if (earlier.cellGroup == leg.cellGroup)
                        throw cRuntimeError("%s entry %d: cell group \"%s\" appears twice in \"legs\"",
                                paramName, i, groupStr.c_str());

                // The RLC mode is the bearer's unless the leg overrides it; the wire format
                // and SN space are RRC's decision at establishment, and a leg states them
                // only to take that decision away from it
                leg.rlcMode = drb.rlcMode;
                if (legEntry && legEntry->containsKey("rlcMode")) {
                    std::string legRlcStr = legEntry->get("rlcMode").stdstringValue();
                    leg.rlcMode = aToRlcMode(legRlcStr);
                    if (leg.rlcMode == TM || leg.rlcMode == UNKNOWN_RLC_MODE)
                        throw cRuntimeError("%s entry %d, leg %d: invalid rlcMode '%s', must be \"UM\" or \"AM\"",
                                paramName, i, j, legRlcStr.c_str());
                }
                if (legEntry && legEntry->containsKey("soFraming"))
                    leg.soFraming = legEntry->get("soFraming").boolValue();
                if (legEntry && legEntry->containsKey("snFieldLength"))
                    leg.snFieldLength = legEntry->get("snFieldLength").intValue();
                drb.legs.push_back(leg);
            }

            // A split bearer's legs are told apart by their position: the leg splitter maps
            // leg 0 onto the master cell group's RLC and leg 1 onto the secondary's (see
            // ~DcPdcpLegSplitter), so they are stated in that order.
            if (drb.legs.size() > 1 && drb.legs.front().cellGroup != MCG)
                throw cRuntimeError("%s entry %d: a split bearer's \"legs\" are stated in cell-group order, \"MCG\" first",
                        paramName, i);
        }

        // primaryPath (optional, default "MCG") and ulDataSplitThreshold (optional bytes,
        // or "infinity"; default infinity): the split-bearer policy (TS 38.331). Meaningful
        // only on a bearer with two legs; the primary path must be one of them. An entry
        // that leaves the legs to RRC may still carry them, for when RRC derives two.
        bool splitPolicyStated = false;
        if (const cValue *v = field("primaryPath")) {
            drb.primaryPath = aToCellGroup(v->stdstringValue());
            if (drb.primaryPath == UNKNOWN_CELL_GROUP)
                throw cRuntimeError("%s entry %d: invalid \"primaryPath\" '%s', must be \"MCG\" or \"SCG\"",
                        paramName, i, v->stdstringValue().c_str());
            splitPolicyStated = true;
        }
        if (const cValue *v = field("ulDataSplitThreshold")) {
            if (v->getType() == cValue::STRING) {
                if (v->stdstringValue() != "infinity")
                    throw cRuntimeError("%s entry %d: \"ulDataSplitThreshold\" must be a byte count or \"infinity\"", paramName, i);
                drb.ulDataSplitThreshold = SPLIT_THRESHOLD_INFINITY;
            }
            else {
                drb.ulDataSplitThreshold = v->intValue();
                if (drb.ulDataSplitThreshold < 0)
                    throw cRuntimeError("%s entry %d: \"ulDataSplitThreshold\" must not be negative", paramName, i);
            }
            splitPolicyStated = true;
        }
        if (splitPolicyStated && !drb.legs.empty() && drb.legs.size() == 1)
            throw cRuntimeError("%s entry %d: \"primaryPath\"/\"ulDataSplitThreshold\" configure a split bearer, "
                    "but this bearer has one leg", paramName, i);
        if (!drb.legs.empty() && drb.legs.size() > 1 &&
                std::none_of(drb.legs.begin(), drb.legs.end(),
                        [&](const RlcBearerDesc& leg) { return leg.cellGroup == drb.primaryPath; }))
            throw cRuntimeError("%s entry %d: \"primaryPath\" %s is not one of the bearer's legs",
                    paramName, i, cellGroupToA(drb.primaryPath).c_str());

        // ulLegSelection / dlLegSelection (optional): the per-direction leg-selection
        // expression over "packetOrdinal" that the leg splitter compiles. Meaningless on a
        // bearer with one leg; an entry that leaves the legs to RRC may carry them, for when
        // RRC derives two.
        if (const cValue *v = field("ulLegSelection")) {
            drb.ulLegSelection = v->stdstringValue();
            if (drb.legs.size() == 1)
                throw cRuntimeError("%s entry %d: \"ulLegSelection\" steers between the legs of a split bearer, "
                        "but this bearer has one leg", paramName, i);
            // Uplink leg selection is consulted only at or above the split threshold; with
            // the threshold at infinity that never happens, so the expression would be dead.
            if (drb.ulDataSplitThreshold == SPLIT_THRESHOLD_INFINITY)
                throw cRuntimeError("%s entry %d: \"ulLegSelection\" is only consulted at or above the split "
                        "threshold, but \"ulDataSplitThreshold\" is infinity -- set it (e.g. 0 to steer every "
                        "uplink PDU by the expression)", paramName, i);
        }
        if (const cValue *v = field("dlLegSelection")) {
            drb.dlLegSelection = v->stdstringValue();
            if (drb.legs.size() == 1)
                throw cRuntimeError("%s entry %d: \"dlLegSelection\" steers between the legs of a split bearer, "
                        "but this bearer has one leg", paramName, i);
            // Downlink has no threshold (the master cannot weigh the secondary's queue), so
            // dlLegSelection needs no threshold companion; it decides every downlink PDU.
        }

        // pduSessionType (optional, default IPv4) and upperProtocol (optional, empty =
        // derive from pduSessionType)
        if (const cValue *v = field("pduSessionType"))
            drb.pduSessionType = aToPduSessionType(v->stdstringValue());
        if (const cValue *v = field("upperProtocol"))
            drb.upperProtocol = v->stdstringValue();

        // rohc (optional): header compression, PDCP-Config headerCompression -- true (every
        // modeled profile), false, or {profiles: [...]}
        if (const cValue *v = field("rohc"))
            drb.rohcProfiles = parseRohcField(*v, paramName, i);
        else if (entry->containsKey("profile") && contains(rohcForDrbProfiles_, entry->get("profile").stdstringValue()))
            drb.rohcProfiles = rohcProfileNames();   // the header compression policy (rohcForDrbProfiles)

        // The entry names its UE by module path (patterns allowed), which is how the
        // configuration follows the UE instead of naming an allocation-order-dependent
        // id; an on-demand entry may omit it to cover every UE
        const char *uePattern = entry->containsKey("ue") ? entry->get("ue").stringValue() : nullptr;
        if (uePattern == nullptr) {
            if (!onDemand)
                throw cRuntimeError("%s entry %d: missing required field \"ue\"", paramName, i);
            uePattern = "**";
        }
        inet::PatternMatcher matcher(uePattern, true, true, true);
        int numMatched = 0;
        for (const auto& [ueModule, nodeIds] : ueNodeIds) {
            std::string path = ueModule->getFullPath();
            if (path.compare(0, networkPrefix.size(), networkPrefix) == 0)
                path.erase(0, networkPrefix.size());
            if (!matcher.matches(path.c_str()))
                continue;
            // A definition describes a bearer of the kind of stack its architecture
            // belongs to, and only those: a "5gc" bearer is selected by QFI and needs
            // SDAP to do the selecting, an "epc" bearer by packet filters and needs a
            // stack without it. A record on the wrong stack could never match, and its
            // isDefault flag would wrongly suppress the auto-default marking above. A
            // pattern (or an omitted "ue") legitimately covers UEs of both kinds, so
            // incompatible UEs are skipped rather than rejected; numMatched counts
            // compatible UEs only, so an entry naming only such UEs still errors.
            if ((drb.coreNetwork == CN_5GC) != ueStackHasSdap(ueModule))
                continue;
            numMatched++;
            if (!onDemand && !drbsOfUe[ueModule].insert({drbId, drb}).second)
                throw cRuntimeError("%s entry %d: DRB %d of UE '%s' is already configured by an earlier entry",
                        paramName, i, (int)num(drbId), path.c_str());

            // Retain the definition for establishment-time matching, its filters
            // compiled; one record per (entry x UE), so an on-demand definition
            // materializes separately per UE
            AuthoredBearer ab;
            ab.ueModule = ueModule;
            ab.desc = drb;
            ab.onDemand = onDemand;
            for (const std::string& spec : drb.filters) {
                auto filter = std::make_unique<inet::PacketFilter>();
                configurePacketFilter(*filter, spec.c_str());
                ab.filters.push_back(std::move(filter));
            }
            authoredBearers_.push_back(std::move(ab));
        }
        if (numMatched == 0 && entry->containsKey("ue"))
            throw cRuntimeError("%s entry %d: its \"ue\" pattern '%s' matches no registered UE with a "
                    "compatible stack", paramName, i, uePattern);
    }
}

std::string BearerConfigurator::relativeToNetwork(const cModule *module) const
{
    // the tables' module-path patterns are relative to the network
    std::string networkPrefix = std::string(getSystemModule()->getFullPath()) + ".";
    std::string path = module->getFullPath();
    if (path.compare(0, networkPrefix.size(), networkPrefix) == 0)
        path.erase(0, networkPrefix.size());
    return path;
}

// Compile the subset of a rule table scoped to one evaluation site: the entries whose
// scope pattern matches the site (an entry without one matches every site), in table
// order, so evaluation stays first-match-wins among the site's rules. matched, if
// given, records the entries the site matched.
static QfiRuleSet compileQfiRulesFor(const cValueArray *table, const char *paramName, const char *scopeField,
        const std::string& sitePath, std::vector<bool> *matched)
{
    QfiRuleSet rules;
    for (int i = 0; i < (int)table->size(); i++) {
        const cValueMap *entry = check_and_cast<const cValueMap *>(table->get(i).objectValue());
        if (entry->containsKey(scopeField)) {
            inet::PatternMatcher matcher(entry->get(scopeField).stringValue(), true, true, true);
            if (!matcher.matches(sitePath.c_str()))
                continue;
        }
        if (matched != nullptr)
            (*matched)[i] = true;
        std::string what = std::string(paramName) + " entry " + std::to_string(i);
        rules.parseRule(entry, what.c_str());
    }
    return rules;
}

QfiRuleSet BearerConfigurator::getDownlinkQfiRules(const cModule *node) const
{
    const cValueArray *table = check_and_cast<const cValueArray *>(par("dlQfiRules").objectValue());
    return compileQfiRulesFor(table, "dlQfiRules", "node", relativeToNetwork(node), nullptr);
}

QfiRuleSet BearerConfigurator::getUplinkQfiRules(const cModule *ue) const
{
    const cValueArray *table = check_and_cast<const cValueArray *>(par("ulQfiRules").objectValue());
    return compileQfiRulesFor(table, "ulQfiRules", "ue", relativeToNetwork(ue), nullptr);
}

void BearerConfigurator::validateQfiRules()
{
    // Validate a whole table up front -- also the entries no site matches, whose
    // errors the per-site compilation would never surface. A rule carries the
    // shared grammar (see QfiRuleSet) plus the table's delivery-scoping column.
    auto validateTable = [](const cValueArray *table, const char *paramName, const char *scopeField) {
        QfiRuleSet scratch;
        for (int i = 0; i < (int)table->size(); i++) {
            const cValueMap *entry = check_and_cast<const cValueMap *>(table->get(i).objectValue());
            std::string what = std::string(paramName) + " entry " + std::to_string(i);
            for (const auto& [key, value] : entry->getFields())
                if (key != "filter" && key != "qfi" && key != "dscpAsQfi" && key != scopeField)
                    throw cRuntimeError("%s: unknown field '%s'", what.c_str(), key.c_str());
            scratch.parseRule(entry, what.c_str());
        }
    };

    // A scoped entry that matches no site is a typo: a "node" pattern names a user
    // plane node (registered with the Binder), a "ue" pattern a registered UE with SDAP
    const cValueArray *dlTable = check_and_cast<const cValueArray *>(par("dlQfiRules").objectValue());
    validateTable(dlTable, "dlQfiRules", "node");
    std::vector<bool> dlMatched(dlTable->size(), false);
    for (const auto& registration : binder_->getUserPlaneNodes())
        compileQfiRulesFor(dlTable, "dlQfiRules", "node", relativeToNetwork(getContainingNode(registration.module)), &dlMatched);
    for (int i = 0; i < (int)dlTable->size(); i++) {
        const cValueMap *entry = check_and_cast<const cValueMap *>(dlTable->get(i).objectValue());
        if (!dlMatched[i] && entry->containsKey("node"))
            throw cRuntimeError("dlQfiRules entry %d: its \"node\" pattern '%s' matches no user plane node "
                    "(a UPF, a PGW or a MEC host's UPF)", i, entry->get("node").stringValue());
    }

    const cValueArray *ulTable = check_and_cast<const cValueArray *>(par("ulQfiRules").objectValue());
    validateTable(ulTable, "ulQfiRules", "ue");
    std::vector<bool> ulMatched(ulTable->size(), false);
    std::set<cModule *> ueModules;
    for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
        if (getNodeTypeById(nodeId) == UE && info.moduleRef != nullptr)
            ueModules.insert(info.moduleRef);
    for (cModule *ueModule : ueModules)
        if (ueStackHasSdap(ueModule))
            compileQfiRulesFor(ulTable, "ulQfiRules", "ue", relativeToNetwork(ueModule), &ulMatched);
    for (int i = 0; i < (int)ulTable->size(); i++) {
        const cValueMap *entry = check_and_cast<const cValueMap *>(ulTable->get(i).objectValue());
        if (!ulMatched[i] && entry->containsKey("ue"))
            throw cRuntimeError("ulQfiRules entry %d: its \"ue\" pattern '%s' matches no registered UE "
                    "with SDAP", i, entry->get("ue").stringValue());
    }
}

void BearerConfigurator::validateStaticDrbs()
{
    // a static definition's bearer is established when its UE's leg attaches (see
    // ConnectionControlEnb::sessionResourceSetup()); a UE attached on no stack has
    // nowhere to establish, which is a configuration error
    for (const AuthoredBearer& ab : authoredBearers_) {
        if (ab.onDemand)
            continue;
        bool attached = false;
        for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
            if (info.moduleRef == ab.ueModule && binder_->getServingNode(nodeId) != NODEID_NONE)
                attached = true;
        if (!attached)
            throw cRuntimeError("staticDrbs: cannot establish DRB %d of UE '%s': the UE is not attached to any cell",
                    (int)num(ab.desc.getDrbId()), ab.ueModule->getFullPath().c_str());
    }
}

const AuthoredBearer *BearerConfigurator::findDrbDefinition(const cModule *ueModule, const inet::Packet *pkt) const
{
    // First matching definition wins, in table order (staticDrbs records are retained
    // ahead of onDemandDrbs ones); the default eps entry catches the flows no filter
    // matched.
    const AuthoredBearer *defaultDef = nullptr;
    for (const AuthoredBearer& ab : authoredBearers_) {
        if (ab.ueModule != ueModule || ab.desc.coreNetwork != CN_EPC)
            continue;
        for (auto& filter : ab.filters)
            if (filter->matches(pkt))
                return &ab;
        if (ab.desc.isDefault && defaultDef == nullptr)
            defaultDef = &ab;
    }
    return defaultDef;
}

const AuthoredBearer *BearerConfigurator::findDrbDefinitionForQfi(const cModule *ueModule, Qfi qfi) const
{
    // One walk, the shape findDrbDefinition() uses for packet filters: the definition
    // that maps this QFI specifically wins immediately, in table order; failing that,
    // the first default definition catches it. authoredBearers_ keeps static records
    // ahead of on-demand ones, so an authored default outranks the onDemandDrbs
    // catch-all.
    const AuthoredBearer *defaultDef = nullptr;
    for (const AuthoredBearer& ab : authoredBearers_) {
        if (ab.ueModule != ueModule || ab.desc.coreNetwork != CN_5GC)
            continue;
        if (contains(ab.desc.mappedQfis, qfi))
            return &ab;
        if (ab.desc.isDefault && defaultDef == nullptr)
            defaultDef = &ab;
    }
    return defaultDef;
}

const AuthoredBearer *BearerConfigurator::findStaticDrbDefinition(const cModule *ueModule, DrbId drbId) const
{
    for (const AuthoredBearer& ab : authoredBearers_)
        if (!ab.onDemand && ab.ueModule == ueModule && ab.desc.getDrbId() == drbId)
            return &ab;
    return nullptr;
}

} //namespace
