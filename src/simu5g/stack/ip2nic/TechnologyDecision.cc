#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include "simu5g/stack/ip2nic/TechnologyDecision.h"
#include "simu5g/stack/ip2nic/HandoverPacketHolderUe.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

using namespace inet;

Define_Module(TechnologyDecision);

void TechnologyDecision::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        lowerLayerOut_ = gate("lowerLayerOut");

        nodeType_ = aToNodeType(par("nodeType").stdstringValue());

        binder_.reference(this, "binderModule", true);

        auto *networkIf = getContainingNicModule(this);
        dualConnectivityEnabled_ = networkIf->par("dualConnectivityEnabled").boolValue();

        if (nodeType_ == NODEB) {
            cModule *bs = getContainingNode(this);
            nodeId_ = MacNodeId(bs->par("macNodeId").intValue());
        }
        else if (nodeType_ == UE) {
            cModule *ue = getContainingNode(this);
            nodeId_ = MacNodeId(ue->par("macNodeId").intValue());
            if (ue->hasPar("nrMacNodeId"))
                nrNodeId_ = MacNodeId(ue->par("nrMacNodeId").intValue());
        }
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
    if (!markPacket(srcAddr, destAddr, tos, useNR)) {
        EV << "TechnologyDecision: UE is not attached to any serving node. Delete packet." << endl;
        delete pkt;
        return;
    }

    pkt->addTagIfAbsent<TechnologyReq>()->setUseNR(useNR);
    send(pkt, lowerLayerOut_);
}

bool TechnologyDecision::markPacket(inet::Ipv4Address srcAddr, inet::Ipv4Address dstAddr, uint16_t typeOfService, bool& useNR)
{
    // In the current version, the Ip2Nic module of the master eNB (the UE) selects which path
    // to follow based on the Type of Service (TOS) field:
    // - use master eNB if tos < 10
    // - use secondary gNB if 10 <= tos < 20
    // - use split bearer if tos >= 20
    //
    // To change the policy, change the implementation of the TechnologyDecision::markPacket() function
    //
    // TODO use a better policy
    // TODO make it configurable from INI or XML?

    if (nodeType_ == NODEB) {
        MacNodeId ueId = binder_->getMacNodeId(dstAddr);
        MacNodeId nrUeId = binder_->getNrMacNodeId(dstAddr);
        bool ueLteStack = (binder_->getServingNodeOrSelf(ueId) != NODEID_NONE);
        bool ueNrStack = (binder_->getServingNodeOrSelf(nrUeId) != NODEID_NONE);

        if (dualConnectivityEnabled_ && ueLteStack && ueNrStack && typeOfService >= 20) { // use split bearer TODO fix threshold
            // even packets go through the LTE eNodeB
            // odd packets go through the gNodeB

            FlowKey key{srcAddr.getInt(), dstAddr.getInt(), typeOfService};
            int sentPackets = splitBearersTable_[key]++;

            if (sentPackets % 2 == 0)
                useNR = false;
            else
                useNR = true;
        }
        else {
            if (ueLteStack && ueNrStack) {
                if (typeOfService >= 10)                                                     // use secondary cell group bearer TODO fix threshold
                    useNR = true;
                else                                                  // use master cell group bearer
                    useNR = false;
            }
            else if (ueLteStack)
                useNR = false;
            else if (ueNrStack)
                useNR = true;
            else
                return false;
        }
    }

    if (nodeType_ == UE) {
        bool ueLteStack = (binder_->getServingNodeOrSelf(nodeId_) != NODEID_NONE);
        bool ueNrStack = (binder_->getServingNodeOrSelf(nrNodeId_) != NODEID_NONE);
        if (dualConnectivityEnabled_ && ueLteStack && ueNrStack && typeOfService >= 20) { // use split bearer TODO fix threshold
            FlowKey key{srcAddr.getInt(), dstAddr.getInt(), typeOfService};
            int sentPackets = splitBearersTable_[key]++;

            if (sentPackets % 2 == 0)
                useNR = false;
            else
                useNR = true;
        }
        else {
            // KLUDGE: this is necessary to prevent a runtime error in one of the simulations:
            //
            //    test_numerology, multicell_CBR_UL, ue[9], t=0.001909132428, event #475 (HandoverPacketHolder), #476 (Ip2Nic)
            //    after: ueLteStack=true, ueNrStack=true, servingNodeId=1, nrServingNodeId=2, typeOfService=10 --> useNr = true (ip2nic.cc#319: UE branch, not dualconn("else") )
            //    before: ueLteStack=true, ueNrStack=true,servingNodeId=1, nrServingNodeId=0, typeOfService=10 --> useNr = false, (HandoverPacketHolder.cc#360)
            //
            auto handoverPacketHolder = check_and_cast<HandoverPacketHolderUe*>(getParentModule()->getSubmodule("handoverPacketHolder"));
            servingNodeId_ = handoverPacketHolder->getServingNodeId();
            nrServingNodeId_ = handoverPacketHolder->getNrServingNodeId();
            // servingNodeId_ = binder_->getServingNode(nodeId_);
            // nrServingNodeId_ = binder_->getServingNode(nrNodeId_);

            if (servingNodeId_ == NODEID_NONE && nrServingNodeId_ != NODEID_NONE)  // 5G-connected only
                useNR = true;
            else if (servingNodeId_ != NODEID_NONE && nrServingNodeId_ == NODEID_NONE)  // LTE-connected only
                useNR = false;
            else // both
                useNR = (typeOfService >= 10);   // use secondary cell group bearer TODO fix threshold
        }
    }
    return true;
}

} //namespace
