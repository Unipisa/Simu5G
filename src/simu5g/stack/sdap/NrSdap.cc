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


#include "NrSdap.h"

#include "simu5g/stack/sdap/packet/NrSdapHeader_m.h"
#include "simu5g/common/QfiTag_m.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include <inet/common/packet/Packet.h>
#include <inet/common/stlutils.h>
#include <inet/common/ProtocolTag_m.h>

namespace simu5g {

Define_Module(NrSdap);


void NrSdap::initialize()
{
    binder_.reference(this, "binderModule", true);

    // Load QFI-to-DRB mapping from drbConfig parameter
    const cValueArray *arr = check_and_cast_nullable<const cValueArray *>(par("drbConfig").objectValue());
    if (arr && arr->size() > 0) {
        qfiContextManager_.loadFromJson(arr);
        EV << "NrSdap: Loaded " << qfiContextManager_.getDrbMap().size() << " DRB entries from drbConfig" << endl;
        for (const auto& [drb, ctx] : qfiContextManager_.getDrbMap())
            EV << "  DRB " << drb << ": " << ctx << endl;
    }

    // Get pointer to reflective QoS table
    reflectiveQosTable.reference(this, "reflectiveQosTableModule", false);

    // Only UE uses reflective QoS table
    std::string nodeRole = par("nodeRole").stdstringValue();
    isUe = (nodeRole == "UE");
    if (!isUe && reflectiveQosTable.getNullable() != nullptr)
        throw cRuntimeError("Only UE may use a reflective QoS table");
}

bool NrSdap::requiresSdapHeader(int drbIndex)
{
    return par("addSdapHeader").boolValue(); // for now -- should come from RRC config
}

bool NrSdap::shouldEnableReflectiveQos(int qfi)
{
    return par("useReflectiveQos").boolValue(); // for now -- should come from RRC config
}

const inet::Protocol *NrSdap::getUpperProtocol(const DrbContext *ctx)
{
    // If an explicit upperProtocol is configured on this DRB, use it
    if (ctx && !ctx->upperProtocol.empty()) {
        const inet::Protocol *proto = inet::Protocol::findProtocol(ctx->upperProtocol.c_str());
        if (!proto)
            throw cRuntimeError("Unknown protocol '%s' in drbConfig upperProtocol", ctx->upperProtocol.c_str());
        return proto;
    }

    // Otherwise derive from pduSessionType
    PduSessionType pduSessionType = ctx ? ctx->pduSessionType : IP_V4;
    switch (pduSessionType) {
        case IP_V4:
        case IP_V4V6:
            return &inet::Protocol::ipv4;
        case IP_V6:
            return &inet::Protocol::ipv6;
        case ETHERNET:
            return &inet::Protocol::ethernetMac;
        case UNSTRUCTURED:
            throw cRuntimeError("Unstructured PDU session requires explicit 'upperProtocol' in drbConfig");
        default:
            throw cRuntimeError("Unknown PDU session type: %d", (int)pduSessionType);
    }
}

void NrSdap::handleMessage(cMessage *msg)
{
    auto arrivalGate = msg->getArrivalGate();
    auto pkt = check_and_cast<inet::Packet *>(msg);

    if (arrivalGate == gate("upperLayerIn"))
        handleUpperPacket(pkt);
    else if (arrivalGate == gate("pdcpIn"))
       handleLowerPacket(pkt);
    else
        throw cRuntimeError("Message arrived on unknown gate: %s", arrivalGate->getFullName());
}

void NrSdap::handleUpperPacket(inet::Packet *pkt)
{
    uint8_t qfi = 0;
    bool qfiFromReflectiveQos = false;

    // Extract QFI from QfiReq tag if present (set by GtpUser from GTP-U header, or by app directly)
    if (pkt->hasTag<QfiReq>()) {
        qfi = pkt->getTag<QfiReq>()->getQfi();
        EV_INFO << "SDAP TX: QFI = " << (int)qfi << " extracted from QfiReq\n";
    }
    else if (isUe) {
        // UE UL: derive QFI from DSCP field of the IP header (DSCP = TOS >> 2)
        // FlowControlInfo.typeOfService is the raw TOS byte set by Ip2Nic::toStackUe()
        // Only applicable for IP PDU sessions; non-IP sessions use QfiReq or default QFI.
        if (pkt->hasTag<FlowControlInfo>()) {
            auto fci = pkt->getTag<FlowControlInfo>();
            uint8_t tos = (uint8_t)fci->getTypeOfService();
            if (tos > 0) {
                uint8_t dscp = tos >> 2;
                qfi = dscp;
                EV_INFO << "SDAP TX: QFI = " << (int)qfi << " derived from DSCP=" << (int)dscp << "\n";
            }
        }
        // If DSCP gave no QFI, try reflective QoS
        if (qfi == 0 && reflectiveQosTable != nullptr) {
            uint8_t reflectiveQfi = reflectiveQosTable->lookupUplinkQfi(pkt);
            if (reflectiveQfi > 0) {
                qfi = reflectiveQfi;
                qfiFromReflectiveQos = true;
                EV_INFO << "SDAP TX: QFI = " << (int)qfi << " derived from reflective QoS\n";
            }
        }
        if (qfi == 0)
            EV_WARN << "SDAP TX: No QFI from DSCP or reflective QoS, defaulting QFI to 0\n";
    }
    else {
        EV_WARN << "SDAP TX: QfiReq not found on gNB path, defaulting QFI to 0\n";
    }

    // Lookup DRB index
    int drbIndex = 0;

    if (isUe) {
        // UE side: simple QFI -> drbIndex lookup
        int idx = qfiContextManager_.getDrbIndexForQfi(qfi);
        if (idx >= 0)
            drbIndex = idx;
        else
            EV_WARN << "SDAP TX: No DRB mapping for QFI=" << (int)qfi << " on UE, using DRB 0\n";
    } else {
        // gNB side: need dest UE nodeId + QFI -> drbIndex
        MacNodeId destUeId = pkt->getTag<FlowControlInfo>()->getDestId();
        if (destUeId == NODEID_NONE)
            EV_WARN << "SDAP TX: destId not set in FlowControlInfo, using DRB 0\n";
        else {
            int idx = qfiContextManager_.getDrbIndex(destUeId, qfi);
            if (idx >= 0)
                drbIndex = idx;
            else {
                // Fallback: use first DRB configured for this UE
                int fallback = qfiContextManager_.getFirstDrbForUe(destUeId);
                if (fallback >= 0) {
                    drbIndex = fallback;
                    EV_WARN << "SDAP TX: No DRB mapping for ueNodeId=" << destUeId << " QFI=" << (int)qfi
                            << ", falling back to first DRB " << drbIndex << " for this UE\n";
                } else {
                    EV_WARN << "SDAP TX: No DRB mapping for ueNodeId=" << destUeId << " QFI=" << (int)qfi
                            << ", no DRBs configured for this UE, using DRB 0\n";
                }
            }
        }
    }

    EV_INFO << "SDAP TX: Selected DRB=" << drbIndex << " for QFI=" << (int)qfi << "\n";

    // Check if SDAP header is required for this DRB
    if (requiresSdapHeader(drbIndex)) {
        // Build SDAP header according to 3GPP TS 37.324
        auto sdapHeader = makeShared<NrSdapHeader>();
        sdapHeader->setQfi(qfi);

        // Enable reflective QoS flag if this QFI supports it and we're not already using reflective QoS
        bool enableReflectiveQos = shouldEnableReflectiveQos(qfi) && !qfiFromReflectiveQos;  //TODO on gNB only?
        sdapHeader->setReflectiveQoS(enableReflectiveQos);

        pkt->insertAtFront(sdapHeader);
        EV_INFO << "SDAP TX: Inserted SDAP header with QFI = " << (int)qfi
                << ", reflectiveQoS = " << (enableReflectiveQos ? "true" : "false") << "\n";
    }
    else {
        EV_INFO << "SDAP TX: No SDAP header required for DRB " << drbIndex << "\n";
    }

    // Set DRB ID and RLC type on FlowControlInfo for PDCP/RLC entity creation and routing
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    lteInfo->setDrbId(DrbId(drbIndex));
    const DrbContext *ctx = qfiContextManager_.getDrbContext(drbIndex);
    if (ctx)
        lteInfo->setRlcType(ctx->rlcType);

    // Establish connection if not yet done for this (drbId, destId) pair
    if (establishedConnections_.insert({DrbId(drbIndex), lteInfo->getDestId()}).second)
        binder_->establishUnidirectionalDataConnection(lteInfo.get());

    // Set protocol tag for outgoing frame to PDCP layer
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::sdap);

    EV_INFO << "SDAP TX: Forwarding to DRB " << drbIndex << "\n";
    send(pkt, "pdcpOut");
}

void NrSdap::handleLowerPacket(inet::Packet *pkt)
{
    auto lteInfo = pkt->findTag<FlowControlInfo>();
    int drbIndex = lteInfo ? num(lteInfo->getDrbId()) : -1;

    EV_INFO << "SDAP RX: Received packet from DRB " << drbIndex << ": " << pkt->peekAtFront() << "\n";

    int qfi = 0;

    // Check if packet has SDAP header (should be at the front according to 3GPP TS 37.324)
    if (requiresSdapHeader(drbIndex)) {
        // Extract SDAP header from the front of the packet
        auto sdapHeader = pkt->removeAtFront<NrSdapHeader>();
        qfi = sdapHeader->getQfi();

        EV_INFO << "SDAP RX: Extracted SDAP header with QFI = " << qfi << "\n";

        // Validate QFI range (0-63 according to 3GPP)
        if (qfi > 63) {
            EV_WARN << "SDAP RX: Invalid QFI value " << qfi << " (should be 0-63)\n";
            qfi = 0; // Use default QFI
        }

        // Handle reflective QoS if UE and enabled
        if (sdapHeader->getReflectiveQoS()) {
            EV_INFO << "SDAP RX: Reflective QoS enabled for QFI " << (int)qfi << "\n";
            if (isUe && reflectiveQosTable != nullptr) {
                reflectiveQosTable->handleDownlinkFlow(pkt, qfi);
            }
        }
    }
    else {
        EV_INFO << "SDAP RX: No SDAP header expected for DRB " << drbIndex << "\n";

        // For DRBs without SDAP header, derive QFI from DRB context (use first QFI in the list)
        const DrbContext* ctx = qfiContextManager_.getDrbContext(drbIndex);
        if (ctx && !ctx->qfiList.empty()) {
            qfi = ctx->qfiList[0];
            EV_INFO << "SDAP RX: Using QFI " << qfi << " from DRB context\n";
        }
    }

    // Validate QFI ↔ DRB consistency
    const DrbContext* ctxValidate = qfiContextManager_.getDrbContext(drbIndex);
    if (ctxValidate) {
        if (!contains(ctxValidate->qfiList, (int)qfi))
            EV_WARN << "SDAP RX: DRB/QFI mismatch! Received on DRB=" << drbIndex << ", QFI=" << qfi << " not in qfiList\n";
    }

    // Add QoS indication tag for upper layers
    auto qosIndTag = pkt->addTagIfAbsent<QfiInd>();
    qosIndTag->setQfi(qfi);

    // Set protocol tag for upper layer based on PDU session type
    const DrbContext* ctxProto = qfiContextManager_.getDrbContext(drbIndex);
    const inet::Protocol *upperProto = getUpperProtocol(ctxProto);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(upperProto);

    EV_INFO << "SDAP RX: Forwarding packet with QFI " << qfi << " to upper layer (protocol: " << upperProto->getName() << ")\n";
    send(pkt, "upperLayerOut");
}


} //namespace
