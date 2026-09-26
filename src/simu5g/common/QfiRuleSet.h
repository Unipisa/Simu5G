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

#ifndef _QFI_RULE_SET_H_
#define _QFI_RULE_SET_H_

#include <memory>
#include <vector>

#include <inet/common/packet/PacketFilter.h>

#include "simu5g/common/LteCommon.h"

namespace omnetpp { class cValueMap; }

namespace simu5g {

/**
 * @brief An ordered set of QFI-assignment rules, shared by every module that
 * classifies packets to QoS flows.
 *
 * The rule grammar is one and the same at both evaluation sites -- the network
 * side's tunnel entry (~TrafficFlowFilter, downlink) and the UE's stack entry
 * (~QosFlowClassifier, uplink): rules are evaluated in order, first match wins.
 * The rules are authored centrally, in the dlQfiRules and ulQfiRules parameters
 * of ~BearerConfigurator, which compiles each site's table and delivers it.
 * Rule fields:
 *  - filter (string, optional): an inet::PacketFilter -- a message-name pattern
 *    (e.g. "*VoIP*") or an expression written as "expr(...)"
 *    (e.g. "expr(has(udp) && udp.destPort == 3000)"); omitted = the rule matches
 *    every packet. An expression is evaluated on every packet of the site, e.g. on
 *    an IPv6 UE's Neighbor Discovery messages too, so it must check for the
 *    headers it reads (has()).
 *  - qfi (int, 0..63) or dscpAsQfi (bool): the QFI to assign -- a fixed value, or
 *    the packet's DSCP field (IPv4 Type of Service or IPv6 Traffic Class) read as
 *    the QFI (exactly one of the two)
 *
 * An Ethernet frame, the payload of an Ethernet session, is classified by the same
 * rules, whose filter expressions read its headers (e.g.
 * "expr(has(ieee8021qctag) && ieee8021qctag.pcp == 5)"); a dscpAsQfi rule does not
 * match it, as it has no DSCP field (see classifyFrame()).
 *
 * A packet matching no rule is left unclassified: classify() returns QFI_NONE,
 * and what that means -- the default flow, or no marking at all -- is the
 * caller's to decide. (The distinction matters because QFI 0 is a real
 * classification, onto the default flow.)
 */
class QfiRuleSet
{
  protected:
    struct QfiRule {
        std::unique_ptr<inet::PacketFilter> filter;   // null = match all
        Qfi qfi = QFI_NONE;
        bool dscpAsQfi = false;
    };
    std::vector<QfiRule> rules_;

  public:
    // Parse and validate one rule and append it; 'what' names the rule in error
    // messages (e.g. "dlQfiRules entry 2"), and errors throw at setup time rather
    // than on the first packet. Only the rule grammar's own fields are read: the
    // full entry schema, including the delivery-scoping columns that ride in the
    // same maps, is the caller's to validate.
    void parseRule(const omnetpp::cValueMap *rule, const char *what);

    // The QFI of the first matching rule, or QFI_NONE if no rule covers the packet;
    // dscp is the packet's DSCP field, the QFI of a dscpAsQfi rule
    Qfi classify(inet::Packet *pkt, uint8_t dscp) const;

    // The same for an Ethernet frame (the payload of an Ethernet session), which has
    // no DSCP field: a dscpAsQfi rule does not match it
    Qfi classifyFrame(inet::Packet *pkt) const;

    bool empty() const { return rules_.empty(); }

    friend std::ostream& operator<<(std::ostream& os, const QfiRuleSet& e)
    {
        os << "rules=" << e.rules_.size();
        for (size_t i = 0; i < e.rules_.size(); i++) {
            const auto& r = e.rules_[i];
            os << " [" << i << ": " << (r.filter ? "filtered" : "matchAll")
               << (r.dscpAsQfi ? " dscpAsQfi" : "") << " qfi=" << r.qfi << "]";
        }
        return os;
    }
};

} // namespace simu5g

#endif
