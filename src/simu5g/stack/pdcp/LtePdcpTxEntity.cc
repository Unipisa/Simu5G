//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/pdcp/LtePdcpTxEntity.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include "simu5g/stack/packetFlowObserver/PacketFlowObserverBase.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include <inet/common/ProtocolTag_m.h>
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/sdap/packet/NrSdapHeader_m.h"


namespace simu5g {

Define_Module(LtePdcpTxEntity);

simsignal_t LtePdcpTxEntity::receivedPacketFromUpperLayerSignal_ = registerSignal("receivedPacketFromUpperLayer");
simsignal_t LtePdcpTxEntity::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");
simsignal_t LtePdcpTxEntity::pdcpSduSentSignal_ = registerSignal("pdcpSduSent");

void LtePdcpTxEntity::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
        nodeId_ = MacNodeId(getContainingNode(this)->par("macNodeId").intValue());

        // the bearer's header compression, as RRC pushed it (see PdcpEntityBase)
        cStringTokenizer profiles(par("rohcProfiles").stringValue());
        if (profiles.hasMoreTokens()) {
            RohcCompressor::Parameters params;
            while (profiles.hasMoreTokens())
                params.profiles.insert(parseRohcProfile(profiles.nextToken()));
            auto readSizes = [this](const char *parName, std::map<RohcProfile, B>& sizes) {
                const cValueMap *map = check_and_cast<const cValueMap *>(par(parName).objectValue());
                for (const auto& [name, value] : map->getFields()) {
                    long size = value.intValue();
                    if (size < 1)
                        throw cRuntimeError("%s: the compressed header size of profile \"%s\" must be at least 1 byte", parName, name.c_str());
                    sizes[parseRohcProfile(name)] = B(size);
                }
            };
            readSizes("rohcFoHeaderSizes", params.foHeaderSize);
            readSizes("rohcSoHeaderSizes", params.soHeaderSize);
            params.irOverhead = B(par("rohcIrOverhead"));
            params.irPackets = par("rohcIrPackets");
            params.foPackets = par("rohcFoPackets");
            params.irRefresh = par("rohcIrRefresh");
            params.foRefresh = par("rohcFoRefresh");
            rohc_ = std::make_unique<RohcCompressor>(params);
        }

        emitPerSduSignals_ = par("emitPerSduSignals");

        rlcType_ = aToRlcMode(par("rlcMode").stringValue());
        switch (rlcType_) {
            case UM: pdcpHeaderLength_ = PDCP_HEADER_UM; break;
            case AM: pdcpHeaderLength_ = PDCP_HEADER_AM; break;
            case TM: pdcpHeaderLength_ = 0; break;
            default:
                throw cRuntimeError("LtePdcpTxEntity::initialize(): invalid rlcMode param '%s'", par("rlcMode").stringValue());
        }
    }
}

void LtePdcpTxEntity::handlePacketFromUpperLayer(Packet *pkt)
{
    emit(receivedPacketFromUpperLayerSignal_, pkt);

    EV << NOW << " LtePdcpTxEntity::handlePacketFromUpperLayer - processing packet " << pkt->getName() << " from IP layer" << endl;

    // perform PDCP operations
    compressHeader(pkt); // header compression

    // PDCP Packet creation
    auto pdcpHeader = makeShared<LtePdcpHeader>();
    pdcpHeader->setSequenceNumber(sno_++); // set sequence number in PDCP header

    // The header size is resolved from this entity's own pushed "rlcMode" param (see initialize()).
    pdcpHeader->setChunkLength(B(pdcpHeaderLength_));
    pkt->trim();
    pkt->insertAtFront(pdcpHeader);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::pdcp);

    EV << "LtePdcpTxEntity::handlePacketFromUpperLayer: Packet size=" << pkt->getByteLength() << "B\n";

    EV << NOW << " LtePdcpTxEntity::handlePacketFromUpperLayer: sending PDCP PDU to the RLC layer" << endl;
    deliverPdcpPdu(pkt);
}

void LtePdcpTxEntity::compressHeader(Packet *pkt)
{
    if (isCompressionEnabled()) {
        // Check PacketProtocolTag to determine packet type
        auto protocolTag = pkt->findTag<PacketProtocolTag>();
        inet::Ptr<inet::Chunk> sdapHeader = nullptr;

        // If packet has SDAP protocol tag, remove SDAP header first
        if (protocolTag && protocolTag->getProtocol() == &LteProtocol::sdap) {
            sdapHeader = pkt->removeAtFront<NrSdapHeader>();
            EV << "LtePdcp : Removed SDAP header before compression\n";
        }

        RohcCompressor::Result result = rohc_->compress(pkt);
        EV << "LtePdcp : Header compression performed, profile " << rohcProfileName(result.profile) << ", CID " << result.cid
           << ", " << rohcStateName(result.state) << ", compressed header " << result.compressedSize << "\n";

        // If we had an SDAP header, add it back on top of the ROHC header
        if (sdapHeader) {
            sdapHeader->markImmutable();
            pkt->insertAtFront(sdapHeader);
            EV << "LtePdcp : Added SDAP header back on top of ROHC header\n";
        }
    }
}


void LtePdcpTxEntity::deliverPdcpPdu(Packet *pdcpPkt)
{
    if (emitPerSduSignals_) {
        auto lteInfo = pdcpPkt->getTag<FlowControlInfo>();
        if (hasListeners(pdcpSduSentSignal_) && lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
            emit(pdcpSduSentSignal_, pdcpPkt);
        }
        emit(sentPacketToLowerLayerSignal_, pdcpPkt);
    }
    send(pdcpPkt, "out");
}

} //namespace

