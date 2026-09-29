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

#include "simu5g/common/QfiRuleSet.h"

namespace simu5g {

using namespace omnetpp;

void QfiRuleSet::parseRule(const cValueMap *entry, const char *what)
{
    QfiRule rule;
    if (entry->containsKey("qfi")) {
        long qfi = entry->get("qfi").intValue();
        if (qfi < 0 || qfi > 63)
            throw cRuntimeError("%s: qfi %ld is outside the 0..63 range", what, qfi);
        rule.qfi = Qfi(qfi);
    }
    if (entry->containsKey("dscpAsQfi"))
        rule.dscpAsQfi = entry->get("dscpAsQfi").boolValue();
    if (entry->containsKey("qfi") == rule.dscpAsQfi)
        throw cRuntimeError("%s: exactly one of \"qfi\" and \"dscpAsQfi\" must be given", what);
    if (entry->containsKey("filter")) {
        rule.filter = std::make_unique<inet::PacketFilter>();
        configurePacketFilter(*rule.filter, entry->get("filter").stringValue());
    }
    rules_.push_back(std::move(rule));
}

Qfi QfiRuleSet::classify(inet::Packet *pkt, uint8_t dscp) const
{
    for (const QfiRule& rule : rules_)
        if (rule.filter == nullptr || rule.filter->matches(pkt))
            return rule.dscpAsQfi ? Qfi(dscp) : rule.qfi;
    return QFI_NONE;
}

Qfi QfiRuleSet::classifyFrame(inet::Packet *pkt) const
{
    for (const QfiRule& rule : rules_)
        if (!rule.dscpAsQfi && (rule.filter == nullptr || rule.filter->matches(pkt)))
            return rule.qfi;
    return QFI_NONE;
}

} // namespace simu5g
