#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include "simu5g/stack/ip2nic/TechnologyDecision.h"
#include "simu5g/stack/ip2nic/Ip2Nic.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(TechnologyDecision);

void TechnologyDecision::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        lowerLayerOut_ = gate("lowerLayerOut");
        ip2nic_ = check_and_cast<Ip2Nic *>(getParentModule()->getSubmodule("ip2nic"));
    }
}

void TechnologyDecision::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);

    auto ipHeader = pkt->peekAtFront<inet::Ipv4Header>();
    auto srcAddr = ipHeader->getSrcAddress();
    auto destAddr = ipHeader->getDestAddress();
    short int tos = ipHeader->getTypeOfService();

    bool useNR;
    if (!ip2nic_->markPacket(srcAddr, destAddr, tos, useNR)) {
        EV << "TechnologyDecision: UE is not attached to any serving node. Delete packet." << endl;
        delete pkt;
        return;
    }

    pkt->addTagIfAbsent<TechnologyReq>()->setUseNR(useNR);
    send(pkt, lowerLayerOut_);
}

} //namespace
