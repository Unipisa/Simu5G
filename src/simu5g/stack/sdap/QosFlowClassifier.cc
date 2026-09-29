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

#include "simu5g/stack/sdap/QosFlowClassifier.h"

#include "simu5g/common/L3Utils.h"
#include "simu5g/common/QfiTag_m.h"

namespace simu5g {

using namespace inet;

Define_Module(QosFlowClassifier);

void QosFlowClassifier::setQfiRules(QfiRuleSet&& rules)
{
    Enter_Method_Silent("setQfiRules");
    qfiRules_ = std::move(rules);
}

void QosFlowClassifier::initialize()
{
    sessionType_ = aToSessionType(par("sessionType").stdstringValue());
}

void QosFlowClassifier::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<Packet *>(msg);

    // An Unstructured session has one QoS flow, the default one: its default QoS rule
    // has no packet filter and applies to every packet (TS 23.501 5.7.1.4), so no rule
    // is evaluated
    if (sessionType_ == UNSTRUCTURED) {
        pkt->addTagIfAbsent<QfiReq>()->setQfi(Qfi(0));
        send(pkt, "lowerLayerOut");
        return;
    }

    // An already-present QFI (an application that set it directly) is not second-guessed.
    // A packet no rule covers stays untagged -- deliberately not tagged with 0, because
    // QFI 0 is a real classification (the default flow) and absence is what lets SDAP
    // fall back to reflective QoS.
    if (!pkt->hasTag<QfiReq>() && !qfiRules_.empty()) {
        // an Ethernet session's frame is classified as it is (TS 23.501 5.7.6.3)
        Qfi qfi = sessionType_ == ETHERNET ? qfiRules_.classifyFrame(pkt) : qfiRules_.classify(pkt, dscpOf(pkt->getTag<IpHeaderFieldsTag>()->getTos()));
        if (qfi != QFI_NONE) {
            pkt->addTag<QfiReq>()->setQfi(qfi);
            EV_INFO << "QosFlowClassifier - " << pkt->getName() << " classified to QFI " << qfi << "\n";
        }
        else
            EV_INFO << "QosFlowClassifier - " << pkt->getName() << " matches no rule, left unclassified\n";
    }

    send(pkt, "lowerLayerOut");
}

} // namespace simu5g
