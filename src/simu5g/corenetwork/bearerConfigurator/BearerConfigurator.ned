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

package simu5g.corenetwork.bearerConfigurator;

//
// Central configurator of the network's data radio bearers.
//
// This module does not model a single 3GPP network function. It combines, in one
// network-wide service, decisions that a real system distributes between the core
// network and the RAN: in 5GS, the SMF controls PDU sessions and QoS flows, while the
// NG-RAN decides how QoS flows map onto DRBs and derives the radio configuration
// (TS 23.501 5.7.1, TS 38.300 12); in EPS, the corresponding roles are spread across
// the MME and the eNB. Here every bearer is authored in one place instead, and the
// signalling that would carry the configuration (N1/N2/S1/RRC procedures) is not
// modeled: within the model, the RAN never authors a bearer of its own -- every
// bearer comes from this module's tables.
//
// One cellular network holds one instance of this module, alongside its ~Binder. Like
// the Binder, it is always reached through module paths supplied in parameters
// (bearerConfiguratorModule), never through hardcoded ones, so several cellular
// networks -- each with its own Binder and BearerConfigurator pair -- can coexist
// within one simulation. The "ue" patterns of the definition tables range over the
// UEs registered in this instance's Binder only, but are matched against module paths
// relative to the toplevel network, so in such a setup the entries (and the
// module-path parameters) name their nodes by qualified paths, e.g. "networkA.ue[*]".
//
// Modeling abstractions and current limitations:
//  - Bearer establishment is a direct call into the RRC of each node involved; no
//    signalling procedure carries it.
//  - Packet-triggered establishment (onDemandDrbs) is a simulation policy. In 5GS, a
//    first UL packet with an unmapped QFI would ride the already-established default
//    DRB, and the gNB could reconfigure the mapping afterwards (TS 38.300 Annex A);
//    creating the DRB before the first packet is delivered is a shortcut.
//  - One PDU session (in EPS terms: one PDN connection) per UE is assumed. QFI
//    mappings, the default DRB and the pduSessionType are therefore scoped per UE,
//    although the spec scopes all of them per PDU session (TS 37.324); a UE with
//    several sessions, e.g. toward multiple DNs or with two session types, cannot
//    be described yet.
//  - The QoS characteristics attached to a definition describe the DRB as a whole.
//    In 5GS they belong to QoS flows, and flows with different QoS profiles may
//    legitimately share a DRB; per-flow QoS is not modeled.
//  - legSelection is a Simu5G extension: 3GPP steers split bearers by PDCP-level
//    controls (primaryPath, ul-DataSplitThreshold), not by per-PDU expressions.
//
// The staticDrbs parameter is the network's data radio bearer configuration: one entry per
// bearer per UE, describing the bearer before and independent of its establishment (modeled
// on the DRB-ToAddMod and RLC-BearerConfig records of TS 38.331). This module tells the RRC
// of each node involved in a bearer -- the UE and its serving node -- what to set up, and
// RRC pushes on what each of its layers consumes. Nothing reads this configuration back out.
//
// A static bearer is not only configured but also established up front, in the last
// initialization stage: each entry results in the same establishment call that
// packet-triggered setup makes, so traffic finds the bearer in place. A dual-stack UE's
// bearer is established on the stack that packet-triggered establishment would pick
// (split into per-technology legs when the UE is served by a node pair in dual
// connectivity); an entry whose UE is attached to no cell at that point is a
// configuration error. To make the static configuration the only source of bearers,
// disable packet-triggered establishment via the establishBearersOnDemand parameter of
// ~NrSdap (or ~Ip2Nic, for stacks without SDAP); see the VoIP-DL-MultiQfi-NoOnDemand
// configuration of the simulations/nr/standalone_drb example.
//
// Because an entry names its UE by module path rather than by node id, the configuration
// follows the UE from cell to cell, and neither end of the radio link is configured
// directly from the ini.
//
// staticDrbs fields:
//  - coreNetwork (string, required): "5gc" or "epc" -- which bearer architecture selects the
//    bearer. A "5gc" bearer is selected by the QFIs mapped onto it (mappedQfis; needs SDAP in
//    the stack); an "epc" bearer is selected by packet filters (filters; for stacks without
//    SDAP). Stated per entry, never inferred; each receiving RRC checks it against its stack.
//  - ue (string): module path of the UE node, relative to the network; patterns are allowed,
//    so one entry can describe a bearer of many UEs (e.g. "ue[*]")
//  - drbId (int): DRB identity, 1..32 (TS 38.331 DRB-Identity)
//  - profile (string, optional): name of a drbProfiles entry supplying defaults for the
//    fields it does not spell out itself
//  - mappedQfis (int[], optional, "5gc" only): QFI values (0..63) mapped to this DRB; an entry without
//    it does not take part in SDAP's QFI-to-DRB mapping. This field maps already-assigned
//    QFIs onto the DRB; which QFI a packet carries is decided by the QFI classification
//    rules (the dlQfiRules and ulQfiRules parameters below)
//  - filters (string[], optional, "epc" only): packet filters selecting this bearer, each
//    an inet::PacketFilter -- a message-name pattern (e.g. "VoIP*") or an expression written
//    as "expr(...)" (e.g. "expr(udp.destPort == 3000)"). An expression sees the packet that
//    triggers establishment, i.e. an UL-triggered and a DL-triggered flow present swapped
//    port/address roles; a list can carry both spellings.
//  - lcg (int, optional): the bearer's logical channel group (0..3;
//    mac-LogicalChannelConfig). In 3GPP an LCG only groups logical-channel buffers
//    for buffer status reporting; that Simu5G's MAC scheduler additionally serves
//    lower LCG indices first is a Simu5G scheduling policy, not spec semantics (the
//    spec's per-logical-channel priority is a separate parameter, not modeled, and
//    NR allows 8 LCGs where this model keeps LTE's 4). Omitted = derived from the
//    QoS profile's priority level (bucketed by lcgPriorityBounds), or 0 when the
//    definition carries no "qosPriorityLevel"
//  - rlcMode (string, optional when the definition carries a "packetErrorRate"): RLC
//    mode ("AM" or "UM"; "TM" is rejected -- 3GPP transparent mode carries
//    BCCH/PCCH/SRB0-class channels, not DRBs); omitted = derived from the QoS
//    profile's packet error rate
//    (packetErrorRate <= amPerThreshold -> "AM", else "UM": a PER target HARQ alone
//    cannot meet gets ARQ).
//    Required, in the entry or its profile, when there is nothing to derive it from.
//  - primaryPath (string, default "MCG"): on a split bearer (two legs), the preferred leg --
//    used in either direction whenever the bearer is not splitting; must be one of its two
//    legs (TS 38.331 PDCP-Config, TS 38.323 5.2.1; applied by ~DcPdcpLegSplitter).
//  - ulDataSplitThreshold (int bytes, or "infinity"; default "infinity"): uplink only. How
//    much of a split bearer's data must be queued waiting to send (in its legs' RLC buffers)
//    before the UE uses the second leg as well as primaryPath. "infinity" means never -- the
//    uplink stays on primaryPath. (Only the UE has both legs' queues locally; downlink has no
//    such threshold, see dlLegSelection.)
//  - ulLegSelection (string, optional): once the uplink is using both legs (at or above the
//    threshold), which leg each PDU takes -- an expr(...) over packetOrdinal (a running count
//    of the bearer's PDUs) returning the leg number, 0 or 1 (e.g. "expr(packetOrdinal % 2)"
//    for round-robin). Default when omitted: the less-loaded leg. Set ulDataSplitThreshold to
//    0 to have it decide every uplink PDU; stating it while leaving the threshold at infinity
//    is rejected.
//  - dlLegSelection (string, optional): the downlink counterpart, applied at the DC master.
//    The master cannot weigh the secondary leg's queue (it is at another node, across X2), so
//    downlink has no threshold or load-balancing: with dlLegSelection each downlink PDU takes
//    the leg it returns, and without it the downlink stays on primaryPath. (Real downlink
//    split flow control is X2-feedback-driven and is not modeled.)
//  - legs (string[]/object[], optional): the cell groups that serve this bearer, i.e. its
//    RLC bearers (TS 38.331 RLC-BearerConfig). One leg = an MCG or an SCG bearer, both =
//    a split bearer (TS 37.340; stated in that order). An element is either a cell group
//    name ("MCG" or "SCG"), or an object naming it and overriding the RLC configuration
//    it inherits from the entry: {leg: "SCG", rlcMode: "AM", soFraming: true,
//    snFieldLength: 18}. Omitted = the configuration does not state the legs and RRC
//    derives them, as it always has. An SCG bearer's PDCP still terminates at the master
//    node -- the core network delivers the UE's traffic there -- and every PDU is
//    relayed to the secondary over X2 (an MN-terminated SCG bearer in TS 37.340 terms).
//  - pduSessionType (string, optional): "IPv4", "IPv6", "IPv4v6", "Ethernet", "Unstructured";
//    default: "IPv4"
//  - upperProtocol (string, optional): INET protocol name for upper layer dispatch (this is
//    normally only needed for "Unstructured" PDU sessions); default: derived from
//    pduSessionType
//  - isDefault (bool, optional): marks this as the default DRB for the UE -- the fallback
//    when no QFI-to-DRB mapping ("5gc") or packet filter ("epc") matches. If no entry of
//    a UE is marked in either table, the UE's single static bearer takes the role; a UE
//    with several bearers must mark one explicitly.
//  - suppressSdapHeader (bool, optional, "5gc" only): the bearer's packets carry no SDAP
//    header (TS 38.331 sdap-HeaderDL/UL "absent"). Only meaningful on the default bearer
//    (any other bearer with at most one mapped QFI is headerless already), and only sound
//    when a single QoS flow actually rides it: the receiver assigns every headerless
//    packet the bearer's sole mapped QFI (QFI 0 when none is mapped), and the sender
//    verifies each packet against that value, stopping with an error when a second flow
//    shows up. The header decision itself is computed by this module and delivered per
//    bearer; SDAP applies it as pushed.
//  - gbr (bool), packetDelayBudget (double, ms), packetErrorRate (double),
//    qosPriorityLevel (int) -- all optional: the
//    QoS profile of the bearer's flows (5QI characteristics). Any of them present marks the
//    bearer as having a QoS profile, which RRC pushes into the eNB/gNB MAC for QoS-aware
//    scheduling.
//
// Field groups that repeat across entries can be named once in the drbProfiles parameter (a
// JSON map of name -> field group) and referenced from entries via their "profile" field. A
// profile describes what the bearer is, so it may hold any entry field except the ones that
// say which UE it belongs to (ue, drbId), which architecture and flows select it (coreNetwork,
// mappedQfis, filters, isDefault).
//
// A commonly used subset of the standardized QoS characteristics rows -- the core 1..9
// of each table; both tables define many more -- is available as predefined profiles,
// referenced by name without being defined: "qci-1".."qci-9" (TS 23.203 Table 6.1.7-A)
// and "5qi-1".."5qi-9" (TS 23.501 Table 5.7.4-1). A row carries what the spec
// standardizes -- gbr, qosPriorityLevel, packetDelayBudget, packetErrorRate -- and
// nothing else: the RLC mode and the logical channel group are RAN choices, derived
// from those characteristics by default (the way a gNB derives its AS configuration
// from the delivered QoS profile; see the rlcMode and lcg field descriptions) and
// stated on the entry only to override, e.g.
// {coreNetwork: "5gc", ue: "ue[*]", drbId: 1, mappedQfis: [1, 2], profile: "5qi-1"}.
// A drbProfiles entry may not redefine a predefined name. NOTE the spec's priority
// scales (QCI 1..9, 5QI 10..90) differ from each other and from the small hand-picked
// values older configurations use. The QoS-aware scheduler weights bearers by
// 1/qosPriorityLevel, so only priority RATIOS matter to it and the two spec catalogs
// discriminate identically; but the absolute values still drive the lcgPriorityBounds
// bucketing (whose defaults suit the 5QI scale: single-digit priorities all land in
// LCG 0), so profiles from different scales still should not be mixed within one
// network.
//
// Example (in an ini file):
//
// <pre>
// *.bearerConfigurator.drbProfiles = {
//     voice:  {rlcMode: "UM", gbr: true, packetDelayBudget: 100, packetErrorRate: 1e-2,
//              qosPriorityLevel: 2}
//     }
// *.bearerConfigurator.staticDrbs = [
//     {coreNetwork: "5gc", ue: "ue[*]", drbId: 1, mappedQfis: [1, 2], profile: "voice"}
//     ]
// </pre>
//
// The onDemandDrbs parameter describes bearers that are created when traffic first
// matches them, instead of being configured up front. Entries have the same fields as
// staticDrbs minus drbId: an on-demand bearer's id is assigned at creation, so
// definitions can never collide over ids ("ue" may also be omitted, meaning every UE).
// A "5gc" entry serves the QFIs in its mappedQfis: the first packet of an uncovered QFI
// creates the DRB (SDAP asks this module on a QFI-to-DRB lookup miss), and later QFIs
// of the same entry join it. An "epc" entry is matched by its filters against the
// packet that triggers bearer establishment, and an entry with isDefault=true catches
// the flows no filter matched ("5gc" entries cannot be the default: the default DRB is
// where unmapped QFIs go, so it must exist up front). A created bearer's definition is
// delivered to the RRCs involved exactly like a staticDrbs entry. Matching order is
// staticDrbs entries first, then onDemandDrbs entries, each in table order, first
// match wins.
//
// A flow that no definition covers is a configuration error: every on-demand bearer's
// properties come from a definition entry, never from the packet. The parameter's
// default value keeps configurations that author nothing working, whichever stack they
// run: for SDAP-less stacks it carries the well-known packet-name classes as ordinary
// catch-all definitions ("VoIP*" = LCG 0, "gaming*" = LCG 2, "VoD*" = LCG 1, everything
// else LCG 3, all RLC UM), and for SDAP stacks a single default bearer (the last row,
// "5gc"), header-suppressed: an unauthored run carries the one default QoS flow with no
// SDAP header on the wire, and a run whose traffic actually shows up with several QFIs
// stops with an error asking for authored bearers (or an unsuppressed default) instead
// of silently mixing its flows. Overriding the parameter replaces the whole set
// with the configuration's own.
// An "epc" entry describes bearers of SDAP-less stacks only and a "5gc" entry those of
// SDAP stacks only; a "ue" pattern (or an omitted "ue") covering UEs of both kinds
// simply skips the incompatible ones, but an entry whose explicit "ue" pattern matches
// no UE with a compatible stack is an error. So in any given network only the rows for
// the stacks present take effect, and the two default rows of the two kinds coexist
// harmlessly: the "5gc" default bearer is what lets a UE with SDAP run unauthored (its
// QFI-to-DRB table would otherwise be empty and the first packet would find no bearer),
// the "epc" rows do the same for a UE without.
// D2D and multicast bearers are outside the definition system: the tables describe
// infrastructure (Uu) bearers, while those are sidelink ones. They are established
// with a fixed configuration (RLC UM, LCG 3). The model has no sidelink bearer
// vocabulary and needs none: the only multicast in the model is D2D groupcast, which
// is kept working as it stands, and the network never originates multicast of its own.
//
// Example (in an ini file; the first entry suits a stack with SDAP, the other two
// one without):
//
// <pre>
// *.bearerConfigurator.onDemandDrbs = [
//     {coreNetwork: "5gc", ue: "ue[*]", mappedQfis: [3, 4], profile: "5qi-3"},
//     {coreNetwork: "epc", filters: ["expr(udp.destPort == 3000)"], lcg: 0, rlcMode: "UM"},
//     {coreNetwork: "epc", isDefault: true, lcg: 3, rlcMode: "UM"}
//     ]
// </pre>
//
// See the VoIP-DL-MultiQfi-OnDemandDrb configuration of simulations/nr/standalone_drb
// ("5gc") and the SingleCell-DL-OnDemandDrb configuration of simulations/lte/tutorial
// ("epc") for working setups.
//
// The dlQfiRules and ulQfiRules parameters author the network's QFI classification:
// which QoS flow (QFI) each packet belongs to. The bearer definitions above map
// already-assigned QFIs onto DRBs (mappedQfis); these two tables are where the QFIs
// themselves are assigned, once per direction at its ingress. The rules are
// delivered from here to their evaluation sites -- dlQfiRules to the
// ~TrafficFlowFilter at each core-network tunnel entry (modeling the PDR/QER rules
// the SMF installs into a UPF over N4 at session setup), ulQfiRules to each UE's
// ~QosFlowClassifier (modeling the QoS rules NAS signaling installs into a UE at
// PDU session establishment, TS 23.501 5.7.1.4) -- and the sites never author rules
// of their own: the ini author writes the operator's classification policy here,
// and this module plays the signaling the model does not have.
//
// Rule fields (both tables; evaluated in order at each site, first match wins):
//  - filter (string, optional): an inet::PacketFilter -- a message-name pattern
//    (e.g. "*VoIP*") or an expression written as "expr(...)"
//    (e.g. "expr(udp.destPort == 3000)"); omitted = the rule matches every packet
//  - qfi (int, 0..63) or dscpAsQfi (bool): the QFI to assign -- a fixed value, or
//    the packet's IPv4 DSCP field read as the QFI (exactly one of the two)
//  - node (string, dlQfiRules only, optional): module-path pattern of the
//    core-network nodes whose tunnel-entry filter gets the rule (relative to the
//    network, e.g. "upf" or "**.upf_mec"); omitted = every such node
//  - ue (string, ulQfiRules only, optional): module-path pattern of the UEs that
//    get the rule, matched like the bearer tables' "ue" field; omitted = every UE
//    with SDAP
//
// The two directions are authored separately, and a conversation that needs
// classification in both states a rule in each table, each written from its own
// direction's perspective. There is no sharing or reversal between them: the same
// filter means different things at the two ends (in "expr(udp.destPort == 3000)"
// the destination port is the UE's application port for downlink traffic and the
// server's for uplink), and a filter expression cannot be reversed mechanically.
// This mirrors the spec's own shape -- downlink PDRs at the UPF, uplink QoS rules
// in the UE.
//
// A downlink packet no rule covers gets QFI 0, the default flow; an uplink packet
// no rule covers is left unclassified, which lets reflective QoS or the default
// DRB take over (see ~QosFlowClassifier). The dlQfiRules default value,
// [{dscpAsQfi: true}], is plain DSCP-as-QFI assignment at every tunnel entry.
// Overriding either parameter replaces its whole table, so a configuration that
// scopes rules to one node (say, a MEC host's UPF) and still wants the DSCP
// classification elsewhere appends the catch-all back explicitly. Base stations
// are not delivery sites -- no SMF installs classification rules into a gNB -- and
// their filters classify unattributed traffic by a built-in DSCP-as-QFI residual
// (see ~TrafficFlowFilter).
//
simple BearerConfigurator
{
    parameters:
        string binderModule = default("binder");
        object staticDrbs = default([]);  // JSON array: [{coreNetwork, ue, drbId, profile, mappedQfis, filters, lcg, rlcMode, legs, pduSessionType, upperProtocol, isDefault, suppressSdapHeader, gbr, packetDelayBudget, packetErrorRate, qosPriorityLevel}, ...]; see banner comment for details
        object onDemandDrbs = default([  // JSON array, staticDrbs schema minus "drbId" (and "ue" optional): bearers created when traffic first matches them; the default value lets both stack kinds run unauthored; see banner comment for details
                {coreNetwork: "epc", filters: ["VoIP*"], lcg: 0, rlcMode: "UM"},
                {coreNetwork: "epc", filters: ["gaming*"], lcg: 2, rlcMode: "UM"},
                {coreNetwork: "epc", filters: ["VoDPacket*", "VoDFinishPacket*"], lcg: 1, rlcMode: "UM"},
                {coreNetwork: "epc", isDefault: true, lcg: 3, rlcMode: "UM"},
                {coreNetwork: "5gc", isDefault: true, suppressSdapHeader: true, lcg: 3, rlcMode: "UM"}
            ]);
        object drbProfiles = default({});  // JSON map of named field groups referenced by the staticDrbs entries' "profile" field, e.g. {voice: {rlcMode: "UM", gbr: true, ...}, ...}; see banner comment

        // QFI classification rules, delivered to their evaluation sites (see banner comment)
        object dlQfiRules = default([{dscpAsQfi: true}]);  // JSON array of downlink QFI-assignment rules [{node, filter, qfi|dscpAsQfi}, ...], delivered to the traffic flow filters at core-network tunnel entries; first match wins per site
        object ulQfiRules = default([]);  // JSON array of uplink QFI-assignment rules [{ue, filter, qfi|dscpAsQfi}, ...], delivered to the UEs' QoS-flow classifiers; [] = classify nothing

        // QoS-derived RAN defaults for definitions that do not state rlcMode/lcg themselves
        // (operator policy, not spec-mandated: 3GPP standardizes the QoS characteristics,
        // the RAN chooses what to make of them)
        double amPerThreshold = default(1e-4);  // rlcMode derivation: a definition whose QoS profile has packetErrorRate <= this gets "AM", else "UM"
        object lcgPriorityBounds = default([25, 50, 75]);  // Lcg derivation: qosPriorityLevel < bounds[k] -> LCG k, else LCG 3; strictly ascending, one bound between each pair of adjacent LCGs
        @display("i=block/cogwheel");
}
