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

#include <inet/common/PatternMatcher.h>
#include <inet/common/stlutils.h>

#include "simu5g/common/InitStages.h"
#include "simu5g/common/QfiRuleSet.h"
#include "simu5g/corenetwork/bearerConfigurator/BearerConfigurator.h"
#include <algorithm>
#include "simu5g/corenetwork/trafficFlowFilter/TrafficFlowFilter.h"
#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/stack/rrc/Registration.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(BearerConfigurator);

// The complete field vocabulary of a bearer-definition entry (staticDrbs/onDemandDrbs);
// anything else in an entry or profile is rejected as a typo
static const std::vector<std::string> KNOWN_ENTRY_FIELDS = {
    "coreNetwork", "ue", "drbId", "profile", "mappedQfis", "filters", "lcg", "rlcMode",
    "legs", "primaryPath", "ulDataSplitThreshold", "ulLegSelection", "dlLegSelection",
    "pduSessionType", "upperProtocol", "isDefault", "suppressSdapHeader",
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
        binder_->subscribe(Binder::nodeUnregisteredSignal_, this);
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        // After INITSTAGE_SIMU5G_NODE_RELATIONSHIPS, so the UEs' serving nodes are known,
        // and before INITSTAGE_SIMU5G_MAC_SCHEDULER_CREATION, where the scheduler takes
        // the address of the QoS map that this fills through RRC.
        configureDrbs();
        deliverQfiRules();
    }
    else if (stage == inet::INITSTAGE_LAST) {
        establishStaticDrbs();
    }
}

bool BearerConfigurator::isDualConnectivityRequired(const FlowId& flow)
{
    MacNodeId sourceId = flow.sourceId;
    MacNodeId destId = flow.destId;

    // Part 1: Check if NodeB is in DC setup
    MacNodeId nodeB = (getNodeTypeById(sourceId) == UE) ? binder_->getServingNode(sourceId) : sourceId;
    ASSERT(nodeB != NODEID_NONE);

    MacNodeId secondaryNode = binder_->getSecondaryNode(nodeB);
    MacNodeId masterNode = binder_->getMasterNodeOrSelf(nodeB);
    bool nodeBInDC = (secondaryNode != NODEID_NONE) || (masterNode != nodeB);

    // Part 2: Check if UE is dual technology capable
    MacNodeId ue = getNodeTypeById(sourceId) == UE ? sourceId :
                   getNodeTypeById(destId) == UE ? destId :
                   NODEID_NONE;

    bool ueIsDualTech = false;  //TODO true? if a nodeB in DC setup sends multicast, can it use dual connectivity?
    if (ue != NODEID_NONE) {
        Registration *reg = check_and_cast<Registration*>(binder_->getRrcByNodeId(ue)->getSubmodule("registration"));
        ueIsDualTech = reg->isDualTechnology();
    }

    return nodeBInDC && ueIsDualTech;
}

DrbId BearerConfigurator::assignDrbId(MacNodeId a, MacNodeId b)
{
    auto pair = std::minmax(a, b);
    auto& inUse = drbIdsInUse_[{pair.first, pair.second}];

    // Lowest free ID: identities released with their bearer are handed out again, which
    // is what keeps the space bounded for a UE that establishes and releases bearers
    // repeatedly (at every handover, say).
    unsigned short id = 1;
    while (inUse.count(DrbId(id)))
        id++;
    if (id > MAX_DRB_ID)
        throw cRuntimeError("BearerConfigurator::assignDrbId - out of DRB identities for the node pair (%hu, %hu): "
                "all %d are in use", num(pair.first), num(pair.second), MAX_DRB_ID);

    inUse.insert(DrbId(id));
    return DrbId(id);
}

void BearerConfigurator::reserveDrbId(MacNodeId a, MacNodeId b, DrbId drbId)
{
    auto pair = std::minmax(a, b);
    drbIdsInUse_[{pair.first, pair.second}].insert(drbId);
}

void BearerConfigurator::releaseDrbId(MacNodeId a, MacNodeId b, DrbId drbId)
{
    auto pair = std::minmax(a, b);
    auto it = drbIdsInUse_.find({pair.first, pair.second});
    if (it != drbIdsInUse_.end() && it->second.erase(drbId) != 0)
        EV << "BearerConfigurator::releaseDrbId - DRB " << drbId << " of the node pair (" << pair.first
           << ", " << pair.second << ") is free again" << endl;
}

bool BearerConfigurator::ownsStaticDrbId(cModule *ueModule, DrbId drbId)
{
    for (const AuthoredBearer& ab : authoredBearers_)
        if (!ab.onDemand && ab.ueModule == ueModule && ab.desc.getDrbId() == drbId)
            return true;
    return false;
}

void BearerConfigurator::forgetOnDemandDrbId(cModule *ueModule, MacNodeId a, MacNodeId b, DrbId drbId)
{
    std::pair<MacNodeId, MacNodeId> pairKey = std::minmax(a, b);
    for (AuthoredBearer& ab : authoredBearers_) {
        if (!ab.onDemand || ab.ueModule != ueModule)
            continue;
        auto it = ab.pairIds.find(pairKey);
        if (it != ab.pairIds.end() && it->second == drbId)
            ab.pairIds.erase(it);
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

        for (auto& [drbId, drb] : drbs) {
            computeUseSdapHeader(drb);
            pushDrbToRrcs(ueModule, drb);
        }
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
static bool ueStackHasSdap(cModule *ueModule)
{
    cModule *nic = ueModule->getSubmodule("cellularNic");
    return nic != nullptr && nic->getSubmodule("sdap") != nullptr;
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

void BearerConfigurator::registerTrafficFlowFilter(TrafficFlowFilter *tff)
{
    Enter_Method_Silent("registerTrafficFlowFilter");
    trafficFlowFilters_.push_back(tff);
}

void BearerConfigurator::deliverQfiRules()
{
    // delivery-site module paths in the rule tables are relative to the network,
    // like the bearer tables' "ue" patterns
    std::string networkPrefix = std::string(getSystemModule()->getFullPath()) + ".";
    auto relativePath = [&](cModule *module) {
        std::string path = module->getFullPath();
        if (path.compare(0, networkPrefix.size(), networkPrefix) == 0)
            path.erase(0, networkPrefix.size());
        return path;
    };

    // Validate a whole table up front -- also the entries no site matches, whose
    // errors the per-site compilation below would never surface. A rule carries the
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

    // Compile the table subset scoped to one delivery site: the entries whose scope
    // pattern matches the site (an entry without one matches every site), in table
    // order, so evaluation stays first-match-wins among the site's rules.
    auto compileFor = [](const cValueArray *table, const char *paramName, const char *scopeField,
                         const std::string& sitePath, std::vector<bool>& matched) {
        QfiRuleSet rules;
        for (int i = 0; i < (int)table->size(); i++) {
            const cValueMap *entry = check_and_cast<const cValueMap *>(table->get(i).objectValue());
            if (entry->containsKey(scopeField)) {
                inet::PatternMatcher matcher(entry->get(scopeField).stringValue(), true, true, true);
                if (!matcher.matches(sitePath.c_str()))
                    continue;
            }
            matched[i] = true;
            std::string what = std::string(paramName) + " entry " + std::to_string(i);
            rules.parseRule(entry, what.c_str());
        }
        return rules;
    };

    // Downlink: each registered tunnel-entry filter gets the rules scoped to its node
    const cValueArray *dlTable = check_and_cast<const cValueArray *>(par("dlQfiRules").objectValue());
    validateTable(dlTable, "dlQfiRules", "node");
    std::vector<bool> dlMatched(dlTable->size(), false);
    for (TrafficFlowFilter *tff : trafficFlowFilters_)
        tff->setQfiRules(compileFor(dlTable, "dlQfiRules", "node", relativePath(tff->getParentModule()), dlMatched));
    for (int i = 0; i < (int)dlTable->size(); i++) {
        const cValueMap *entry = check_and_cast<const cValueMap *>(dlTable->get(i).objectValue());
        if (!dlMatched[i] && entry->containsKey("node"))
            throw cRuntimeError("dlQfiRules entry %d: its \"node\" pattern '%s' matches no core-network node "
                    "with a traffic flow filter", i, entry->get("node").stringValue());
    }

    // Uplink: each SDAP UE's classifier gets the rules scoped to it, through its RRC
    const cValueArray *ulTable = check_and_cast<const cValueArray *>(par("ulQfiRules").objectValue());
    validateTable(ulTable, "ulQfiRules", "ue");
    std::vector<bool> ulMatched(ulTable->size(), false);
    std::map<cModule *, std::vector<MacNodeId>> ueNodeIds;
    for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
        if (getNodeTypeById(nodeId) == UE && info.moduleRef != nullptr)
            ueNodeIds[info.moduleRef].push_back(nodeId);
    for (const auto& [ueModule, nodeIds] : ueNodeIds) {
        if (!ueStackHasSdap(ueModule))
            continue;   // no SDAP, no uplink QoS-flow classification
        auto *ueRrc = check_and_cast<BearerManagement *>(binder_->getRrcByNodeId(nodeIds.front())->getSubmodule("bearerManagement"));
        ueRrc->setUplinkQfiRules(compileFor(ulTable, "ulQfiRules", "ue", relativePath(ueModule), ulMatched));
    }
    for (int i = 0; i < (int)ulTable->size(); i++) {
        const cValueMap *entry = check_and_cast<const cValueMap *>(ulTable->get(i).objectValue());
        if (!ulMatched[i] && entry->containsKey("ue"))
            throw cRuntimeError("ulQfiRules entry %d: its \"ue\" pattern '%s' matches no registered UE "
                    "with SDAP", i, entry->get("ue").stringValue());
    }
}

void BearerConfigurator::pushDrbToRrcs(cModule *ueModule, const DrbDesc& drb)
{
    // node ids of the UE module, one per stack (see configureDrbs())
    std::vector<MacNodeId> nodeIds;
    for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
        if (getNodeTypeById(nodeId) == UE && info.moduleRef == ueModule)
            nodeIds.push_back(nodeId);
    ASSERT(!nodeIds.empty());

    DrbId drbId = drb.getDrbId();

    // The UE keys its bearers by "my serving node" (NODEID_NONE), its serving
    // node by the UE. A dual-stack UE has one bearer per stack id, and the
    // serving node of each stack is told about the one that is its own.
    auto *ueRrc = check_and_cast<BearerManagement *>(binder_->getRrcByNodeId(nodeIds.front())->getSubmodule("bearerManagement"));
    DrbDesc ueDrb = drb;
    ueDrb.key = DrbKey(NODEID_NONE, drbId);
    ueRrc->configureDrb(ueDrb);

    for (MacNodeId ueId : nodeIds) {
        MacNodeId servingNodeId = binder_->getServingNode(ueId);
        if (servingNodeId == NODEID_NONE)
            continue;   // this stack is not attached to a cell
        DrbDesc enbDrb = drb;
        enbDrb.key = DrbKey(ueId, drbId);
        auto *enbRrc = check_and_cast<BearerManagement *>(binder_->getRrcByNodeId(servingNodeId)->getSubmodule("bearerManagement"));
        enbRrc->configureDrb(enbDrb);

        // The configuration names the bearer, so its id is taken out of the pool
        // that assignDrbId() hands out to bearers that are not configured here
        reserveDrbId(ueId, servingNodeId, drbId);
    }
}

void BearerConfigurator::establishStaticDrbs()
{
    for (AuthoredBearer& ab : authoredBearers_) {
        if (ab.onDemand)
            continue;

        // the UE's registered node id(s) -- one per stack
        MacNodeId lteUeId = NODEID_NONE, nrUeId = NODEID_NONE;
        for (const auto& [nodeId, info] : binder_->getNodeInfoMap())
            if (info.moduleRef == ab.ueModule)
                (num(nodeId) >= NR_UE_MIN_ID ? nrUeId : lteUeId) = nodeId;

        // select the UE's stack, with the same default that packet-triggered
        // establishment uses (see Ip2Nic::assignBearer): the technology-neutral LTE id
        // when the serving nodes form a DC setup (so that establishDataConnection()
        // splits the bearer into legs), the NR id otherwise. A stack that is not
        // attached is skipped, like in the configuration push (pushDrbToRrcs()); a UE
        // attached on no stack has nowhere to establish, which is an error.
        bool lteAttached = lteUeId != NODEID_NONE && binder_->getServingNode(lteUeId) != NODEID_NONE;
        bool nrAttached = nrUeId != NODEID_NONE && binder_->getServingNode(nrUeId) != NODEID_NONE;
        if (!lteAttached && !nrAttached)
            throw cRuntimeError("staticDrbs: cannot establish DRB %d of UE '%s': the UE is not attached to any cell",
                    (int)num(ab.desc.getDrbId()), ab.ueModule->getFullPath().c_str());
        MacNodeId lteNodeB = lteAttached ? binder_->getServingNode(lteUeId) : NODEID_NONE;
        bool dcSetup = lteNodeB != NODEID_NONE &&
                (binder_->getSecondaryNode(lteNodeB) != NODEID_NONE || binder_->getMasterNodeOrSelf(lteNodeB) != lteNodeB);
        MacNodeId ueId = (lteAttached && nrAttached && dcSetup) ? lteUeId :
                         nrAttached ? nrUeId : lteUeId;

        FlowId flow;
        flow.sourceId = ueId;
        flow.destId = binder_->getServingNode(ueId);
        flow.direction = UL;
        flow.drbId = ab.desc.getDrbId();

        EV << "BearerConfigurator::establishStaticDrbs - establishing DRB " << flow.drbId << " of UE '"
           << ab.ueModule->getFullPath() << "' (nodeId=" << ueId << ") towards serving node "
           << flow.destId << endl;
        establishDataConnection(flow, BearerRequest{ab.desc.rlcMode, ab.desc.lcg});
    }
}

DrbId BearerConfigurator::establishOnDemandBearer(const FlowId& flow, const FlowBindingKey& key, const inet::Packet *pkt)
{
    Enter_Method_Silent("establishOnDemandBearer");

    // D2D and multicast bearers are outside the definition system (definitions
    // describe infrastructure bearers), and are established with a fixed transitional
    // configuration -- RLC UM on LCG 3, the non-GBR default bearer's group -- until
    // they get definitions of their own.
    if (flow.d2dGroupId != NODEID_NONE || flow.d2dTxPeerId != NODEID_NONE || flow.d2dRxPeerId != NODEID_NONE)
        return establishDataConnection(flow, BearerRequest{UM, Lcg(3), key});

    // The requester brings identity only; the bearer's properties come from the
    // definition the flow matches. First matching definition wins, in table order
    // (staticDrbs records are retained ahead of onDemandDrbs ones); the default eps
    // entry catches the flows no filter matched.
    MacNodeId ueId = getNodeTypeById(flow.sourceId) == UE ? flow.sourceId : flow.destId;
    cModule *ueModule = binder_->getNodeModule(ueId);
    if (ueModule != nullptr) {
        AuthoredBearer *defaultDef = nullptr;
        for (auto& ab : authoredBearers_) {
            if (ab.ueModule != ueModule || ab.desc.coreNetwork != CN_EPC)
                continue;
            for (auto& filter : ab.filters)
                if (filter->matches(pkt))
                    return establishFromDefinition(ab, flow, key);
            if (ab.desc.isDefault && defaultDef == nullptr)
                defaultDef = &ab;
        }
        if (defaultDef != nullptr)
            return establishFromDefinition(*defaultDef, flow, key);
    }

    // Every on-demand bearer's properties come from a definition entry, never from
    // the packet; the onDemandDrbs default value carries catch-all definitions, so
    // only a configuration that replaced them with a non-covering set can get here.
    throw cRuntimeError("no bearer definition covers packet '%s' of UE '%s' (nodeId=%d) -- an on-demand "
            "bearer requires a covering staticDrbs/onDemandDrbs entry",
            pkt->getName(), ueModule ? ueModule->getFullPath().c_str() : "?", (int)num(ueId));
}

DrbId BearerConfigurator::establishFromDefinition(AuthoredBearer& ab, const FlowId& flowIn, const FlowBindingKey& key)
{
    FlowId flow = flowIn;

    if (!ab.onDemand) {
        // A static definition's id is pinned, and its descriptor was delivered to the
        // RRCs at initialization: the flow simply joins the configured bearer.
        flow.drbId = ab.desc.getDrbId();
    }
    else {
        // An on-demand DRB id is pair-scoped, so the definition materializes once per
        // node pair: the first match within a pair assigns the pair's lowest free id
        // and delivers the definition to the RRCs involved, and later flows matching
        // the definition join that bearer. After a handover the new pair assigns
        // afresh, and a torn-down bearer's id returns to its pool (see
        // forgetOnDemandDrbId()) -- exactly the identity lifecycle of a bearer nobody
        // authored.
        std::pair<MacNodeId, MacNodeId> pairKey = std::minmax(flow.sourceId, flow.destId);
        auto it = ab.pairIds.find(pairKey);
        if (it == ab.pairIds.end()) {
            DrbId drbId = assignDrbId(flow.sourceId, flow.destId);
            it = ab.pairIds.insert({pairKey, drbId}).first;
            DrbDesc desc = ab.desc;
            desc.key = DrbKey(NODEID_NONE, drbId);
            desc.lcid = LogicalCid(num(drbId));
            EV << "BearerConfigurator::establishFromDefinition - on-demand definition materialized as DRB " << drbId
               << " for UE " << ab.ueModule->getFullPath() << endl;
            pushDrbToRrcs(ab.ueModule, desc);
        }
        flow.drbId = it->second;
    }
    return establishDataConnection(flow, BearerRequest{ab.desc.rlcMode, ab.desc.lcg, key});
}

DrbId BearerConfigurator::resolveDrbForQfi(MacNodeId ueNodeId, Qfi qfi)
{
    Enter_Method_Silent("resolveDrbForQfi");

    cModule *ueModule = binder_->getNodeModule(ueNodeId);
    if (ueModule == nullptr)
        return DRBID_NONE;

    // One walk, the shape establishOnDemandBearer() uses for packet filters: the
    // definition that maps this QFI specifically wins immediately, in table order;
    // failing that, the first default definition catches it. authoredBearers_ keeps
    // static records ahead of on-demand ones, so an authored default outranks the
    // onDemandDrbs catch-all.
    AuthoredBearer *defaultDef = nullptr;
    for (AuthoredBearer& ab : authoredBearers_) {
        if (ab.ueModule != ueModule || ab.desc.coreNetwork != CN_5GC)
            continue;
        if (contains(ab.desc.mappedQfis, qfi))
            return drbOfDefinition(ab, ueNodeId);
        if (ab.desc.isDefault && defaultDef == nullptr)
            defaultDef = &ab;
    }
    if (defaultDef != nullptr)
        return drbOfDefinition(*defaultDef, ueNodeId);
    return DRBID_NONE;
}

DrbId BearerConfigurator::drbOfDefinition(AuthoredBearer& ab, MacNodeId ueNodeId)
{
    // A static definition's bearer was established up front under its pinned id
    if (!ab.onDemand)
        return ab.desc.getDrbId();

    // An on-demand definition materializes once per node pair, like
    // establishFromDefinition(): the id is assigned and the descriptor delivered to the
    // RRCs involved (so it also reaches SDAP's QFI-to-DRB table), and later lookups join
    // the bearer already made.
    MacNodeId servingNodeId = binder_->getServingNode(ueNodeId);
    if (servingNodeId == NODEID_NONE)
        return DRBID_NONE;   // not attached, nowhere to create the bearer
    std::pair<MacNodeId, MacNodeId> pairKey = std::minmax(ueNodeId, servingNodeId);
    auto it = ab.pairIds.find(pairKey);
    if (it == ab.pairIds.end()) {
        DrbId drbId = assignDrbId(ueNodeId, servingNodeId);
        it = ab.pairIds.insert({pairKey, drbId}).first;
        DrbDesc desc = ab.desc;
        desc.key = DrbKey(NODEID_NONE, drbId);
        desc.lcid = LogicalCid(num(drbId));
        EV << "BearerConfigurator::drbOfDefinition - on-demand DRB " << drbId
           << " materialized at UE " << ab.ueModule->getFullPath() << endl;
        pushDrbToRrcs(ab.ueModule, desc);
    }
    return it->second;
}

DrbId BearerConfigurator::establishDataConnection(const FlowId& flowIn, const BearerRequest& reqIn)
{
    Enter_Method_Silent("establishDataConnection");

    // Assign the bearer's DRB id unless the requester brought one (SDAP and the
    // static definitions name their bearers explicitly). IDs are unique per node
    // pair; for multicast the "pair" is (sender, group), there being no single peer.
    FlowId flow = flowIn;
    MacNodeId peerId = (flow.d2dGroupId != NODEID_NONE) ? flow.d2dGroupId : flow.destId;
    if (flow.drbId == DRBID_NONE) {
        flow.drbId = assignDrbId(flow.sourceId, peerId);
        EV << "BearerConfigurator::establishDataConnection - new DRB ID assigned: " << flow.drbId << endl;
    }
    else
        reserveDrbId(flow.sourceId, peerId, flow.drbId);   // named by the requester; keep assignDrbId off it

    // A request that states no RLC mode (SDAP's, for one) takes it, and the LCG, from
    // the bearer's definition entry. Definition entries always state their RLC mode,
    // so the request RRC receives is always concrete.
    BearerRequest req = reqIn;
    if (req.rlcMode == UNKNOWN_RLC_MODE) {
        const DrbDesc *def = findBearerDefinition(flow);
        if (def == nullptr)
            throw cRuntimeError("bearer establishment for DRB %d carries no RLC mode, and no definition "
                    "entry names that DRB -- a request that states no configuration is only valid for "
                    "definition-covered bearers", (int)num(flow.drbId));
        req.rlcMode = def->rlcMode;
        req.lcg = def->lcg;
    }

    bool dualConnected = isDualConnectivityRequired(flow);
    if (!dualConnected) {
        // Without a secondary cell group there is nothing to carry an SCG leg
        if (const DrbDesc *def = findBearerDefinition(flow))
            if (std::any_of(def->legs.begin(), def->legs.end(),
                    [](const RlcBearerDesc& leg) { return leg.cellGroup == SCG; }))
                throw cRuntimeError("BearerConfigurator: the definition of DRB %d states an SCG leg, "
                        "but the flow's UE is not served in dual connectivity -- there is no secondary "
                        "cell group to carry it", (int)num(flow.drbId));
        createConnection(flow, req, true);
    }
    else {
        MacNodeId sourceId = flow.sourceId;
        MacNodeId destId = flow.destId;
        bool isGroupcast = flow.d2dGroupId != NODEID_NONE;

        // Get UE registration if any endpoint is UE
        Registration *ueReg = (getNodeTypeById(sourceId) == UE) ? check_and_cast<Registration*>(binder_->getRrcByNodeId(sourceId)->getSubmodule("registration")) :
                     (!isGroupcast && getNodeTypeById(destId) == UE) ? check_and_cast<Registration*>(binder_->getRrcByNodeId(destId)->getSubmodule("registration")) :
                     nullptr;

        // Which of the UE's two ids belongs to which cell group. A UE's stacks pair with
        // their serving nodes by technology, so the master node's technology decides:
        // under EN-DC the master is an eNB and the master cell group is the UE's LTE
        // stack; under NE-DC the master is a gNB and it is the NR stack.
        //
        // The master's technology is asked of the node itself, not of the UE's current
        // attachment. Attachment moves during a handover, and a stack whose serving node
        // is mid-change matches neither cell group for as long as that lasts.
        MacNodeId masterNodeB = binder_->getMasterNodeOrSelf(getNodeTypeById(sourceId) == UE ? destId : sourceId);
        bool masterIsNr = binder_->isNrNodeB(masterNodeB);
        MacNodeId ueMcgId = ueReg ? (masterIsNr ? ueReg->getNrNodeId() : ueReg->getLteNodeId()) : NODEID_NONE;
        MacNodeId ueScgId = ueReg ? (masterIsNr ? ueReg->getLteNodeId() : ueReg->getNrNodeId()) : NODEID_NONE;

        // A bearer whose definition states its legs is established on those and no others:
        // an MCG bearer never reaches the secondary node, and an SCG bearer's traffic is
        // carried by no cell group of the master's own -- though the master still
        // terminates its PDCP, since the core network delivers the UE's traffic there.
        // A definition that leaves the legs to RRC gets both legs, as before.
        bool hasMcgLeg = true, hasScgLeg = true;
        if (const DrbDesc *def = findBearerDefinition(flow)) {
            if (!def->legs.empty()) {
                hasMcgLeg = std::any_of(def->legs.begin(), def->legs.end(),
                        [](const RlcBearerDesc& leg) { return leg.cellGroup == MCG; });
                hasScgLeg = std::any_of(def->legs.begin(), def->legs.end(),
                        [](const RlcBearerDesc& leg) { return leg.cellGroup == SCG; });
            }
        }

        // Master cell group connection
        FlowId lteFlow = flow;
        lteFlow.sourceId = getNodeTypeById(sourceId) == UE ?
                            ueMcgId :
                            binder_->getMasterNodeOrSelf(sourceId);
        if (!isGroupcast) {  // Only set destId for unicast
            lteFlow.destId = getNodeTypeById(destId) == UE ?
                              ueMcgId :
                              binder_->getMasterNodeOrSelf(destId);
        }
        if (hasMcgLeg)
            createConnection(lteFlow, req, true);
        else {
            // An SCG bearer: only the master's own ends are established -- its RRC wires
            // the PDCP legs to the X2 path instead of local RLC (see
            // BearerManagement::createOutgoingConnection()) -- and the UE's MCG stack is
            // not involved at all. The flow keeps the anchor (MCG) ids: they are what the
            // master's PDCP is keyed and addressed by, and the leg splitter maps them to
            // the SCG per PDU, exactly as on a split bearer's secondary leg.
            ASSERT(!isGroupcast);   // definitions never cover D2D/multicast flows
            FlowId revFlow = lteFlow.reversed();
            BearerRequest revReq = req;
            if (revReq.flowBindingKey.has_value())
                revReq.flowBindingKey = revReq.flowBindingKey->reversed();
            if (lteFlow.sourceId == masterNodeB) {
                createOutgoingConnectionOnNode(masterNodeB, lteFlow, req, true);
                createIncomingConnectionOnNode(masterNodeB, revFlow, revReq, true);
            }
            else {
                createIncomingConnectionOnNode(masterNodeB, lteFlow, req, true);
                createOutgoingConnectionOnNode(masterNodeB, revFlow, revReq, true);
            }
        }

        // Secondary cell group connection
        FlowId nrFlow = flow;
        nrFlow.sourceId = getNodeTypeById(sourceId) == UE ?
                           ueScgId :
                           binder_->getSecondaryNode(binder_->getMasterNodeOrSelf(sourceId));
        if (!isGroupcast) {  // Only set destId for unicast
            nrFlow.destId = getNodeTypeById(destId) == UE ?
                             ueScgId :
                             binder_->getSecondaryNode(binder_->getMasterNodeOrSelf(destId));
        }
        if (hasScgLeg)
            createConnection(nrFlow, req, false);
    }
    return flow.drbId;
}

// The definition a flow's bearer was authored from, if any: the entry whose UE and DRB id
// the flow names. Definitions describe infrastructure bearers only, so a D2D or multicast
// flow never has one.
const DrbDesc *BearerConfigurator::findBearerDefinition(const FlowId& flow)
{
    if (flow.d2dGroupId != NODEID_NONE || flow.d2dTxPeerId != NODEID_NONE || flow.d2dRxPeerId != NODEID_NONE)
        return nullptr;
    MacNodeId ueId = (getNodeTypeById(flow.sourceId) == UE) ? flow.sourceId : flow.destId;
    if (getNodeTypeById(ueId) != UE)
        return nullptr;
    cModule *ueModule = binder_->getNodeModule(ueId);
    std::pair<MacNodeId, MacNodeId> pairKey = std::minmax(flow.sourceId, flow.destId);
    for (const AuthoredBearer& ab : authoredBearers_) {
        if (ab.ueModule != ueModule)
            continue;
        if (!ab.onDemand && ab.desc.getDrbId() == flow.drbId)
            return &ab.desc;
        if (ab.onDemand) {
            // an on-demand definition's id is per node pair (see establishFromDefinition())
            auto it = ab.pairIds.find(pairKey);
            if (it != ab.pairIds.end() && it->second == flow.drbId)
                return &ab.desc;
        }
    }
    return nullptr;
}

void BearerConfigurator::receiveSignal(cComponent *source, simsignal_t signalID, long nodeId, cObject *details)
{
    ASSERT(signalID == Binder::nodeUnregisteredSignal_);
    MacNodeId id = MacNodeId(nodeId);

    // the pools are keyed by node pair, so the departed id sits in every pair it took part in
    for (auto it = drbIdsInUse_.begin(); it != drbIdsInUse_.end(); ) {
        if (it->first.first == id || it->first.second == id)
            it = drbIdsInUse_.erase(it);
        else
            ++it;
    }

    // The remembered multicast flows are keyed by group but owned by their sender: drop the
    // ones this node established, or multicastGroupJoined() would keep handing later joiners
    // an RX leg keyed to a sender that no longer transmits -- and, since the RX descriptor's
    // MacCid carries that sender's id, the PDUs of whichever node took over the group would
    // then arrive on a connection the joiner has no descriptor for. A replacement sender's
    // createConnection() stores a fresh flow, so the group keeps working.
    for (auto it = multicastFlows_.begin(); it != multicastFlows_.end(); ) {
        if (it->first.second == id)
            it = multicastFlows_.erase(it);
        else
            ++it;
    }
}

void BearerConfigurator::createConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    MacNodeId sourceId = flow.sourceId;
    MacNodeId destId = flow.destId;
    MacNodeId groupId = flow.d2dGroupId;

    EV << "BearerConfigurator::establishDataConnection - establishing connection from sourceId=" << sourceId
       << " to destId=" << destId << " groupId=" << groupId << endl;

    bool sourceIsEnb = getNodeTypeById(sourceId) == NODEB;
    bool destIsEnb = getNodeTypeById(destId) == NODEB;
    ASSERT(!sourceIsEnb || !destIsEnb);  // they cannot be both NodeBs

    bool sourceWithPdcp = getNodeTypeById(sourceId)==UE || withPdcp;
    createOutgoingConnectionOnNode(sourceId, flow, req, sourceWithPdcp);

    if (groupId == NODEID_NONE) {
        bool destWithPdcp = getNodeTypeById(destId)==UE || withPdcp;
        createIncomingConnectionOnNode(destId, flow, req, destWithPdcp);

        // A DRB is bidirectional (TS 38.331): create the reverse leg of the bearer
        // at both endpoints as well, so reverse traffic -- user data or RLC-AM
        // STATUS PDUs -- finds its entities in place instead of establishing a
        // separate unidirectional bearer.
        // The reverse leg is the same bearer with the same configuration, seen from the
        // other end -- including the flow key, which the peer binds as IT sees the flow
        // (addresses swapped, direction reversed).
        FlowId revFlow = flow.reversed();
        BearerRequest revReq = req;
        if (revReq.flowBindingKey.has_value())
            revReq.flowBindingKey = revReq.flowBindingKey->reversed();
        createOutgoingConnectionOnNode(destId, revFlow, revReq, destWithPdcp);
        createIncomingConnectionOnNode(sourceId, revFlow, revReq, sourceWithPdcp);
    }
    else {
        // Remember the flow so that nodes joining this group later still get an RX leg; the
        // loop below can only reach the members that already exist. See multicastGroupJoined().
        auto flowKey = std::make_pair(groupId, sourceId);
        if (multicastFlows_.find(flowKey) == multicastFlows_.end())
            multicastFlows_[flowKey] = { flow, req, withPdcp };

        // Multicast bearers stay unidirectional: TX at the sender, RX at the members
        for (auto& [nodeId,_] : binder_->getNodeInfoMap())  //TODO use lte ones if LTE in DC setup, and NR ones if NR in DC setup
            if (nodeId != sourceId && binder_->isInMulticastGroup(nodeId, groupId))
                createIncomingConnectionOnNode(nodeId, flow, req, getNodeTypeById(nodeId)==UE || withPdcp);
    }
}


void BearerConfigurator::multicastGroupJoined(MacNodeId nodeId, MacNodeId groupId)
{
    Enter_Method("multicastGroupJoined(%hu, %hu)", (unsigned short)nodeId, (unsigned short)groupId);

    for (auto& [key, mf] : multicastFlows_) {
        auto& [flowGroupId, senderId] = key;
        if (flowGroupId != groupId || senderId == nodeId)
            continue;
        createIncomingConnectionOnNode(nodeId, mf.flow, mf.req,
                getNodeTypeById(nodeId) == UE || mf.withPdcp);
    }
}

void BearerConfigurator::createIncomingConnectionOnNode(MacNodeId nodeId, const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    BearerManagement *bm = check_and_cast<BearerManagement*>(binder_->getRrcByNodeId(nodeId)->getSubmodule("bearerManagement"));
    bm->createIncomingConnection(flow, req, withPdcp);
}

void BearerConfigurator::createOutgoingConnectionOnNode(MacNodeId nodeId, const FlowId& flow, const BearerRequest& req, bool withPdcp)
{
    BearerManagement *bm = check_and_cast<BearerManagement*>(binder_->getRrcByNodeId(nodeId)->getSubmodule("bearerManagement"));
    bm->createOutgoingConnection(flow, req, withPdcp);
}

} //namespace
