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
#include "simu5g/common/L3Utils.h"
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
    connectionControl_.reference(this, "connectionControlModule", true);

    // Get pointer to reflective QoS table
    reflectiveQosTable.reference(this, "reflectiveQosTableModule", false);

    // Only UE uses reflective QoS table
    std::string nodeRole = par("nodeRole").stdstringValue();
    isUe = (nodeRole == "UE");
    establishBearersOnDemand_ = par("establishBearersOnDemand").boolValue();
    reflectiveQosOverridesQfi_ = par("reflectiveQosOverridesQfi").boolValue();
    if (!isUe && reflectiveQosTable.getNullable() != nullptr)
        throw cRuntimeError("Only UE may use a reflective QoS table");
}

void NrSdap::configureDrb(const DrbDesc& drb)
{
    Enter_Method("configureDrb(drb %d)", (int)num(drb.getDrbId()));
    EV << "NrSdap::configureDrb - " << drb << endl;
    drbTable_.addOrUpdateDrb(drb);
}

void NrSdap::bearerEstablished(DrbKey key)
{
    Enter_Method_Silent("bearerEstablished()");
    EV << "NrSdap::bearerEstablished - " << key << endl;
    establishedBearers_.insert(key);
}

void NrSdap::bearerReleased(DrbKey key)
{
    Enter_Method_Silent("bearerReleased()");
    EV << "NrSdap::bearerReleased - " << key << endl;
    establishedBearers_.erase(key);
}

bool NrSdap::isD2dFlow(inet::Packet *pkt)
{
    // A flow of the D2D machinery, in EITHER mode: an infrastructure-mode packet of
    // a D2D-capable pair still carries the peer/group ids, and its bearer is the
    // same hardcoded sidelink-style one -- the mode switch tears down and
    // re-establishes within that machinery, so ownership must not change with the
    // mode.
    auto lteInfo = pkt->findTag<FlowControlInfo>();
    if (lteInfo == nullptr)
        return false;
    return lteInfo->getD2dTxPeerId() != NODEID_NONE || lteInfo->getD2dRxPeerId() != NODEID_NONE
            || lteInfo->getD2dGroupId() != NODEID_NONE;
}

Qfi NrSdap::recoveryQfi(const DrbDesc *drb)
{
    // The QFI the RX side assigns to a headerless packet: the bearer's sole mapped
    // QFI, or the default flow's QFI 0 when none is mapped (the header-suppressed
    // catch-all default bearer). The TX side checks every headerless packet against
    // the same value, so the two ends agree without a header on the wire.
    ASSERT(drb->mappedQfis.size() <= 1);  // with several mapped QFIs the header is never omitted
    return drb->mappedQfis.empty() ? Qfi(0) : drb->mappedQfis[0];
}

bool NrSdap::shouldEnableReflectiveQos(Qfi qfi)
{
    return par("useReflectiveQos").boolValue(); // for now -- should come from RRC config
}

const inet::Protocol *NrSdap::getUpperProtocol(const DrbDesc *ctx, inet::Packet *pkt)
{
    // If an explicit upperProtocol is configured on this DRB, use it
    if (ctx && !ctx->upperProtocol.empty()) {
        const inet::Protocol *proto = inet::Protocol::findProtocol(ctx->upperProtocol.c_str());
        if (!proto)
            throw cRuntimeError("Unknown protocol '%s' in the DRB's upperProtocol configuration", ctx->upperProtocol.c_str());
        return proto;
    }

    // Otherwise derive from pduSessionType. 3GPP does not signal the IP version per
    // packet: an IP session's receiver reads it from the datagram, and the session
    // type only says which versions may occur.
    PduSessionType pduSessionType = ctx ? ctx->pduSessionType : IP_V4;
    switch (pduSessionType) {
        case IP_V4:
        case IP_V6:
        case IP_V4V6: {
            const inet::Protocol *protocol = &ipProtocolOf(pkt);
            if ((pduSessionType == IP_V4 && protocol != &inet::Protocol::ipv4) || (pduSessionType == IP_V6 && protocol != &inet::Protocol::ipv6))
                throw cRuntimeError("%s datagram received on a DRB of an %s PDU session", protocol->getDescriptiveName(), pduSessionType == IP_V4 ? "IPv4" : "IPv6");
            return protocol;
        }
        case ETHERNET:
            return &inet::Protocol::ethernetMac;
        case UNSTRUCTURED:
            throw cRuntimeError("Unstructured PDU session requires explicit 'upperProtocol' in the DRB configuration");
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
    // D2D flows are outside SDAP: their bearers are peer-keyed, carry no QoS flows,
    // sit outside the bearer-definition system, and Ip2Nic assigns them (3GPP
    // sidelink has its own SL-SDAP over PC5, which is not modeled) -- pass the
    // packet on untouched, exactly as the SDAP-less wiring would deliver it.
    if (isD2dFlow(pkt)) {
        EV_INFO << "SDAP TX: D2D-machinery packet, passing through untouched\n";
        send(pkt, "pdcpOut");
        return;
    }

    Qfi qfi = QFI_NONE;
    bool qfiFromReflectiveQos = false;

    if (!isUe) {
        // gNB DL: the QFI always arrives from the GTP-U header, via GtpUser's QfiReq tag
        if (!pkt->hasTag<QfiReq>())
            throw cRuntimeError("SDAP TX: QfiReq tag missing on gNB DL path -- GtpUser should always set it");
        qfi = pkt->getTag<QfiReq>()->getQfi();
        EV_INFO << "SDAP TX: QFI = " << qfi << " extracted from QfiReq\n";
    }
    else {
        // UE UL: two sources may answer -- the classified QFI (a QfiReq tag, stamped by
        // the QosFlowClassifier's rules or set by the application directly) and a
        // reflective QoS match, the rule derived from observed downlink traffic. When
        // both answer, the reflectiveQosOverridesQfi switch says which wins; one alone
        // is used as-is; deliberately no per-rule precedence between the two kinds.
        Qfi taggedQfi = pkt->hasTag<QfiReq>() ? pkt->getTag<QfiReq>()->getQfi() : QFI_NONE;
        Qfi reflectiveQfi = (reflectiveQosTable != nullptr) ? reflectiveQosTable->lookupUplinkQfi(pkt) : QFI_NONE;
        if (taggedQfi != QFI_NONE && (reflectiveQfi == QFI_NONE || !reflectiveQosOverridesQfi_)) {
            qfi = taggedQfi;
            EV_INFO << "SDAP TX: QFI = " << qfi << " extracted from QfiReq\n";
        }
        else if (reflectiveQfi != QFI_NONE) {
            qfi = reflectiveQfi;
            qfiFromReflectiveQos = true;
            EV_INFO << "SDAP TX: QFI = " << qfi << " derived from reflective QoS\n";
        }
        else {
            // unclassified traffic joins the default QoS flow
            qfi = Qfi(0);
            EV_WARN << "SDAP TX: unclassified packet joins the default QoS flow (QFI 0)\n";
        }
    }

    // Lookup DRB context: nodeId is NODEID_NONE on UE, destUeId on gNB
    MacNodeId nodeId = NODEID_NONE;
    if (!isUe) {
        nodeId = pkt->getTag<FlowControlInfo>()->getDestId();
        if (nodeId == NODEID_NONE)
            EV_WARN << "SDAP TX: destId not set in FlowControlInfo\n";
    }

    const DrbDesc *drb = drbTable_.getDrbForQfi(nodeId, qfi);
    if (drb != nullptr && !establishedBearers_.count(DrbKey(pkt->getTag<FlowControlInfo>()->getDestId(), drb->getDrbId()))) {
        // The mapping names a bearer that is not established -- torn down at a handover
        // or a D2D mode switch. An on-demand definition's id is pair-scoped and was
        // released with the bearer, so the mapped id may belong to the past: treat the
        // QFI as unmapped and re-resolve below -- the control plane materializes the
        // definition for the current node pair and pushes the fresh descriptor back
        // into this table.
        drb = nullptr;
    }
    if (!drb) {
        // The QFI is not in this node's table: ask the control plane which DRB it
        // resolves to -- the definition that maps it, or the UE's default bearer, an
        // on-demand one being materialized (and pushed back into this table) on first
        // use. The control plane is the sole bearer-selection authority; SDAP holds no
        // default-DRB fallback of its own. It works in UE-id space: on the gNB that is
        // nodeId (the destination), on the UE itself the flow's source.
        MacNodeId ueId = isUe ? pkt->getTag<FlowControlInfo>()->getSourceId() : nodeId;
        DrbId drbId = connectionControl_->resolveDrbForQfi(ueId, qfi);
        if (drbId != DRBID_NONE)
            drb = drbTable_.getDrb(DrbKey(nodeId, drbId));
    }
    if (!drb)
        throw cRuntimeError("SDAP TX: no DRB for QFI %d at nodeId=%d -- no definition maps it and no default bearer is configured",
                (int)num(qfi), (int)num(nodeId));

    EV_INFO << "SDAP TX: Selected DRB=" << drb->getDrbId() << " for QFI=" << qfi << "\n";

    // Whether this bearer's packets carry an SDAP header is the control plane's
    // decision, delivered in the descriptor (see DrbDesc::useSdapHeader)
    if (drb->useSdapHeader) {
        // Build SDAP header according to 3GPP TS 37.324
        auto sdapHeader = makeShared<NrSdapHeader>();
        sdapHeader->setQfi(qfi);

        // Enable reflective QoS flag if this QFI supports it and we're not already using reflective QoS
        bool enableReflectiveQos = shouldEnableReflectiveQos(qfi) && !qfiFromReflectiveQos;  //TODO on gNB only?
        sdapHeader->setReflectiveQoS(enableReflectiveQos);

        pkt->insertAtFront(sdapHeader);
        // an SDAP header is now the packet's outermost protocol (PDCP's header
        // compression reads this)
        pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::sdap);
        EV_INFO << "SDAP TX: Inserted SDAP header with QFI = " << qfi
                << ", reflectiveQoS = " << (enableReflectiveQos ? "true" : "false") << "\n";
    }
    else {
        // No header goes on the wire, so the RX side will assign this packet the
        // bearer's recovery QFI; a packet classified differently would silently
        // change QoS flows in transit
        if (qfi != recoveryQfi(drb))
            throw cRuntimeError("SDAP TX: misconfigured DRB %d: it carries no SDAP header, so the "
                    "receiver assigns its packets QFI %d, but this packet has QFI %d -- more than one "
                    "QoS flow rides the bearer; map the extra flows to bearers of their own, or clear "
                    "\"suppressSdapHeader\"",
                    (int)num(drb->getDrbId()), (int)num(recoveryQfi(drb)), (int)num(qfi));
        EV_INFO << "SDAP TX: No SDAP header required for DRB " << drb->getDrbId() << "\n";
    }

    // Set DRB ID on FlowControlInfo for PDCP/RLC entity creation and routing
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    lteInfo->setDrbId(drb->getDrbId());

    // Establish the connection unless the bearer already exists: bearers torn down at
    // handover or at a D2D mode switch are re-established by the next packet, even for
    // an already-seen (drbId, destId) pair.
    if (!establishedBearers_.count(DrbKey(lteInfo->getDestId(), drb->getDrbId()))) {
        if (!establishBearersOnDemand_)
            throw cRuntimeError("SDAP TX: no established bearer for DRB %d (peer nodeId=%d), and on-demand bearer establishment is disabled -- missing or mismatched staticDrbs entry?",
                    (int)num(drb->getDrbId()), (int)num(lteInfo->getDestId()));
        // SDAP decides neither the LCG nor the RLC mode: the control plane resolves both from
        // the bearer's definition entry at establishment (see
        // ConnectionControlEnb::establishBearer()), so the request carries no configuration
        // at all.
        connectionControl_->establishBearer(lteInfo->toFlowId(), BearerRequest{UNKNOWN_RLC_MODE});
    }

    EV_INFO << "SDAP TX: Forwarding to DRB " << drb->getDrbId() << "\n";
    send(pkt, "pdcpOut");
}

void NrSdap::handleLowerPacket(inet::Packet *pkt)
{
    // D2D flows are outside SDAP (see handleUpperPacket()): no SDAP header, no QoS
    // flow, no descriptor -- forward untouched
    if (isD2dFlow(pkt)) {
        EV_INFO << "SDAP RX: D2D-machinery packet, passing through untouched\n";
        send(pkt, "upperLayerOut");
        return;
    }

    auto lteInfo = pkt->findTag<FlowControlInfo>();
    DrbId drbId = lteInfo ? lteInfo->getDrbId() : DRBID_NONE;
    MacNodeId ueId = (!isUe && lteInfo) ? lteInfo->getSourceId() : NODEID_NONE;
    const DrbDesc *drb = (drbId != DRBID_NONE) ? drbTable_.getDrb(DrbKey(ueId, drbId)) : nullptr;
    if (!drb)
        throw cRuntimeError("SDAP RX: Unknown DRB %d (ueId=%d) -- missing entry in bearerConfigurator.staticDrbs?",
                            (int)num(drbId), (int)num(ueId));

    EV_INFO << "SDAP RX: Received packet from DRB " << drbId << ": " << pkt->peekAtFront() << "\n";

    Qfi qfi = QFI_NONE;

    // Whether the packet carries an SDAP header (at the front, 3GPP TS 37.324) is the
    // control plane's per-bearer decision, delivered in the descriptor -- the same
    // field the TX side applied, so the two ends agree by construction
    if (drb->useSdapHeader) {
        // Extract SDAP header from the front of the packet
        auto sdapHeader = pkt->removeAtFront<NrSdapHeader>();
        qfi = sdapHeader->getQfi();

        EV_INFO << "SDAP RX: Extracted SDAP header with QFI = " << qfi << "\n";

        // Validate QFI range (0-63 according to 3GPP)
        if (num(qfi) > 63) {
            EV_WARN << "SDAP RX: Invalid QFI value " << qfi << " (should be 0-63)\n";
            qfi = QFI_NONE;
        }

        // Handle reflective QoS if UE and enabled
        if (sdapHeader->getReflectiveQoS()) {
            EV_INFO << "SDAP RX: Reflective QoS enabled for QFI " << qfi << "\n";
            if (isUe && reflectiveQosTable != nullptr) {
                reflectiveQosTable->handleDownlinkFlow(pkt, qfi);
            }
        }
    }
    else {
        // No header on the wire: assign the recovery QFI, the one value the TX-side
        // check guarantees is the packet's classified QFI
        qfi = recoveryQfi(drb);
        EV_INFO << "SDAP RX: No SDAP header expected for DRB " << drbId << ", using QFI " << qfi << " from DRB context\n";
    }

    // Validate QFI <-> DRB consistency; the default bearer is exempt -- carrying
    // QFIs mapped nowhere is its job
    if (!drb->isDefault && !contains(drb->mappedQfis, qfi))
        EV_WARN << "SDAP RX: DRB/QFI mismatch! Received on DRB=" << drbId << ", QFI=" << qfi << " not in mappedQfis\n";

    // Add QoS indication tag for upper layers
    auto qosIndTag = pkt->addTagIfAbsent<QfiInd>();
    qosIndTag->setQfi(qfi);

    // Set protocol tag for upper layer (see getUpperProtocol())
    const inet::Protocol *upperProto = getUpperProtocol(drb, pkt);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(upperProto);

    EV_INFO << "SDAP RX: Forwarding packet with QFI=" << qfi << " to upper layer (protocol: " << upperProto->getName() << ")\n";
    send(pkt, "upperLayerOut");
}


} //namespace
