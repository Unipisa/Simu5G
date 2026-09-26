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

#ifndef _BEARERCONFIGURATOR_H_
#define _BEARERCONFIGURATOR_H_

#include <map>
#include <memory>
#include <set>
#include <vector>

#include <inet/common/ModuleRefByPar.h>
#include <inet/common/packet/PacketFilter.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/QfiRuleSet.h"
#include "simu5g/stack/rrc/DrbDesc.h"

namespace simu5g {

using namespace omnetpp;

/**
 * The network's bearer definitions: the operator's configuration of the data radio
 * bearers, one instance per cellular network. It parses the definition tables into
 * records the base stations establish bearers from, and holds the QFI classification
 * rule tables the CoreControl delivers. Read-only for everyone once initialized.
 * See the NED file for details.
 */
class BearerConfigurator : public cSimpleModule
{
  public:
    ~BearerConfigurator() override;

    // A bearer definition retained for establishment-time authoring: the UE it belongs
    // to, the descriptor delivered to the RRCs, and its compiled packet filters. One
    // record per (entry x matched UE); records keep table order, staticDrbs before
    // onDemandDrbs, which is the match order of ConnectionControlEnb::establishBearer().
    // An onDemandDrbs record carries no DRB id of its own (desc.key stays DRBID_NONE):
    // like every DRB id, an on-demand bearer's identity is pair-scoped, assigned by
    // the base station at the definition's first match within each node pair and
    // returned to the pair's pool with the bearer -- so a UE that moves to another
    // serving node materializes the definition afresh there.
    struct AuthoredBearer {
        cModule *ueModule = nullptr;
        DrbDesc desc;                  // key = (NODEID_NONE, drbId); DRBID_NONE for onDemand
        bool onDemand = false;         // true = onDemandDrbs entry (ids assigned at first match, per pair)
        bool rohcByPolicy = false;     // desc.rohcProfiles come from the rohcForDrbProfiles policy, not from the entry's "rohc" field
        std::vector<std::unique_ptr<inet::PacketFilter>> filters;   // compiled desc.filters

        friend std::ostream& operator<<(std::ostream& os, const AuthoredBearer& e)
        {
            // ueModule is not dereferenced here: it is not reset when a UE departs the
            // simulation, so it can go stale while this record is still in authoredBearers_
            os << "ue=" << static_cast<const void *>(e.ueModule) << " drbKey=" << e.desc.key
               << " onDemand=" << e.onDemand << " filters=" << e.filters.size();
            return os;
        }
    };

  protected:
    inet::ModuleRefByPar<Binder> binder_;

    // QoS-derived RAN defaults for definitions that state no rlcMode/lcg themselves:
    // the amPerThreshold and lcgPriorityBounds parameters (see the NED documentation)
    double amPerThreshold_ = 0;
    std::vector<long> lcgPriorityBounds_;

    // Header compression policy for definitions that do not state "rohc": the profile
    // names whose bearers get ROHC (the rohcForDrbProfiles parameter)
    std::set<std::string> rohcForDrbProfiles_;

    std::vector<AuthoredBearer> authoredBearers_;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override { throw cRuntimeError("This module does not process messages"); }

    // Parse the data radio bearers described by the staticDrbs and onDemandDrbs
    // parameters (see NED documentation) into authoredBearers_. Nothing is pushed
    // from here: the base station serving a UE installs and establishes the UE's
    // static bearers when the UE attaches (ConnectionControlEnb::sessionResourceSetup()).
    virtual void configureDrbs();

    // A static definition's UE must be attached on some stack at the end of
    // initialization, or its bearer has nowhere to be established: a configuration error
    virtual void validateStaticDrbs();

    // The standardized QoS characteristics rows as built-in drbProfiles entries
    // ("qci-1".."qci-9", "5qi-1".."5qi-9"); built lazily, owned by this module
    omnetpp::cValueMap *predefinedDrbProfiles_ = nullptr;
    virtual const omnetpp::cValueMap *getPredefinedDrbProfiles();

    // Settle the SDAP header decision of one bearer record (TS 38.331 sdap-HeaderDL/UL,
    // stored in DrbDesc::useSdapHeader): a header is needed exactly when the bearer
    // alone cannot name the arriving QFI on the RX side -- it is the default bearer or
    // several QFIs map to it -- unless the configuration suppresses it, asserting that
    // a single QoS flow rides the bearer (which SDAP verifies per packet). Called once
    // the UE's default bearer is settled, since the decision hangs on isDefault.
    virtual void computeUseSdapHeader(DrbDesc& drb);

    // Parse one bearer-definition table (staticDrbs or onDemandDrbs) into
    // authoredBearers_, and, for staticDrbs, into drbsOfUe, where the UE's default
    // bearer is settled.
    virtual void parseDrbDefinitions(const char *paramName, bool onDemand,
            const std::map<cModule *, std::vector<MacNodeId>>& ueNodeIds, const std::string& networkPrefix,
            std::map<cModule *, std::map<DrbId, DrbDesc>>& drbsOfUe);

    // Validate the two QFI rule tables at initialization: every entry's field vocabulary
    // and grammar, and that every scoped entry matches a site -- a "node" pattern a
    // user plane node (registered with the Binder), a "ue" pattern a
    // registered UE with SDAP -- since a pattern that matches nothing is a typo the
    // per-site compilation (getDownlinkQfiRules() etc.) would never surface.
    virtual void validateQfiRules();

    // A module's path relative to the network, which the tables' patterns are matched against
    virtual std::string relativeToNetwork(const cModule *module) const;

  public:
    // Whether the UE's stack contains SDAP. Structure, not configuration: the sdap
    // submodule exists iff the NIC's hasSdap is set, the same resolvability test
    // BearerManagement::configureDrb() applies on its own side.
    static bool ueStackHasSdap(const cModule *ueModule);

    // ---- the bearer definitions, read by the base stations ----

    // Every retained definition, in table order (staticDrbs before onDemandDrbs), of
    // every UE; the records keep their addresses for the whole run
    virtual const std::vector<AuthoredBearer>& getBearerDefinitions() const { return authoredBearers_; }

    // The "epc" definition of the UE that an uplink or downlink packet of the UE
    // matches: the first entry, in table order, whose packet filter matches; failing
    // that, the UE's default entry; nullptr if none covers it
    virtual const AuthoredBearer *findDrbDefinition(const cModule *ueModule, const inet::Packet *pkt) const;

    // The "5gc" definition of the UE that maps the QFI: the first entry, in table
    // order, that names it; failing that, the UE's default entry (it carries the QFIs
    // no other bearer maps); nullptr if none
    virtual const AuthoredBearer *findDrbDefinitionForQfi(const cModule *ueModule, Qfi qfi) const;

    // The staticDrbs entry of the UE that names the DRB id, or nullptr. A static
    // definition owns its id for the whole run, so teardown must not release it.
    virtual const AuthoredBearer *findStaticDrbDefinition(const cModule *ueModule, DrbId drbId) const;

    // ---- the QFI rule tables, read by CoreControl ----

    // The downlink QFI classification rules of a user plane node: the dlQfiRules
    // entries whose "node" pattern matches the node's path relative to the network (an
    // entry without one matches every node), in table order. Read by CoreControl, which
    // installs them into the node over N4; this module delivers nothing itself.
    virtual QfiRuleSet getDownlinkQfiRules(const cModule *node) const;

    // The uplink QFI classification rules of a UE: the ulQfiRules entries scoped to
    // it, likewise. Read by CoreControl, which delivers them to the UE's classifier as
    // the QoS rules of session establishment.
    virtual QfiRuleSet getUplinkQfiRules(const cModule *ue) const;

    // Whether an entry of the ulQfiRules table names the given UE by its "ue" scope,
    // i.e. a QoS rule authored for the UE rather than for every UE
    virtual bool hasUplinkQfiRulesScopedTo(const cModule *ue) const;

    // Whether such an entry, scoped to the given UE, is a dscpAsQfi rule
    virtual bool hasUplinkDscpAsQfiRuleScopedTo(const cModule *ue) const;
};

} //namespace

#endif
