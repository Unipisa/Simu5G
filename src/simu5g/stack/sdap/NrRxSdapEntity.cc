//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


#include "NrRxSdapEntity.h"

#include "simu5g/stack/sdap/packet/NrSdapPdu_m.h"
#include "simu5g/stack/sdap/common/QosTag_m.h"
#include "inet/common/packet/Packet.h"
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>

namespace simu5g {

Define_Module(NrRxSdapEntity);


void NrRxSdapEntity::initialize()
{
    std::string configFile = par("qfiContextFile").stdstringValue();
    contextManager.loadFromFile(configFile);
}

void NrRxSdapEntity::handleMessage(cMessage *msg)
{
    auto arrivalGate = msg->getArrivalGate();
    auto pkt = check_and_cast<inet::Packet *>(msg);

    uint8_t qfi = 0;

    if (arrivalGate == gate("DataPort$i")) {
        EV_INFO << "NrRxSdapEntity: Packet from IP → PDCP\n";
        send(msg, "stackSdap$o");
    }
    else if (strcmp(arrivalGate->getBaseName(), "stackSdap") == 0) {
        int drbIndex = arrivalGate->getIndex();

        EV_INFO << "Before IP: " << pkt->peekAtFront() << "\n";

        auto ipHeader = pkt->removeAtFront<inet::Ipv4Header>();
        int transportProtocol = ipHeader->getProtocolId();

        EV_INFO << "Before Transport: " << pkt->peekAtFront() << "\n";

        if (IP_PROT_TCP == transportProtocol) {
            auto tcpHeader = pkt->removeAtFront<tcp::TcpHeader>();

            EV_INFO << "Before SDAP: " << pkt->peekAtFront() << "\n";
            auto sdapHeader = pkt->removeAtFront<NrSdapPdu>();
            qfi = sdapHeader->getQfi();

            EV_INFO << "SDAP RX: Extracted SDAP header with QFI = " << (int)qfi << "\n";

            // Reinsert headers
            tcpHeader->setChunkLength(tcpHeader->getHeaderLength());
            ipHeader->setTotalLengthField(ipHeader-> getTotalLengthField() - inet::B(sdapHeader->getChunkLength()));

            pkt->insertAtFront(tcpHeader);
        }
        else if (IP_PROT_UDP == transportProtocol) {
            auto udpHeader = pkt->removeAtFront<UdpHeader>();

            EV_INFO << "Before SDAP: " << pkt->peekAtFront() << "\n";
            auto sdapHeader = pkt->removeAtFront<NrSdapPdu>();
            qfi = sdapHeader->getQfi();

            EV_INFO << "SDAP RX: Extracted SDAP header with QFI = " << (int)qfi << "\n";

            // Update total lengths after removing SDAP
            udpHeader->setChunkLength(inet::UDP_HEADER_LENGTH);
            udpHeader->setTotalLengthField(udpHeader->getTotalLengthField() - inet::B(sdapHeader->getChunkLength()));
            ipHeader->setTotalLengthField(ipHeader-> getTotalLengthField() - inet::B(sdapHeader->getChunkLength()));

            // Reinsert headers
            pkt->insertAtFront(udpHeader);
        }
        else {
            EV_WARN << "SDAP RX: Unknown transport protocol\n";
        }

        // Lookup QFI ↔ DRB consistency check
        const QfiContext* ctx = contextManager.getContextByDrb(drbIndex);
        if (ctx && ctx->qfi != qfi) {
            EV_WARN << "SDAP RX: DRB/QFI mismatch! DRB=" << drbIndex << ", received QFI=" << (int)qfi << ", expected QFI=" << (int)ctx->qfi << "\n";
        }

        ipHeader->setChunkLength(ipHeader->getHeaderLength());

        pkt->insertAtFront(ipHeader);
        EV_INFO << "IP: " << pkt->peekAtFront() << "\n";

        send(pkt, "DataPort$o");
    }
    else {
        EV_ERROR << "NrRxSdapEntity: Unknown arrival gate\n";
        delete msg;
    }
}

} //namespace
