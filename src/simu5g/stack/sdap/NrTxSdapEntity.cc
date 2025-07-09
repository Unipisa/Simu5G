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

#include "NrTxSdapEntity.h"

#include "packet/NrSdapPdu_m.h"
#include "common/QosTag_m.h"
#include "inet/common/packet/Packet.h"
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>

namespace simu5g {

Define_Module(NrTxSdapEntity);

void NrTxSdapEntity::initialize()
{
    contextManager = QfiContextManager::getInstance();  // singleton
    std::string configFile = par("qfiContextFile").stdstringValue();
    // will be shared with all, just instantiate QfiContextManager
    contextManager->loadFromFile(configFile); // here we load the config file
    ASSERT(contextManager != nullptr);
}

void NrTxSdapEntity::handleMessage(cMessage *msg)
{
    auto arrivalGate = msg->getArrivalGate();
    auto pkt = check_and_cast<inet::Packet *>(msg);

    if (arrivalGate == gate("DataPort$i")) {

        uint8_t qfi = 0;

        // Extract QFI from QosTagReq if present
        if (pkt->hasTag<simu5g::QosTagReq>()) {
            qfi = pkt->getTag<simu5g::QosTagReq>()->getQfi();
            EV_INFO << "SDAP TX: QFI = " << (int)qfi << " extracted from QosTagReq\n";
        } else {
            EV_WARN << "SDAP TX: QosTagReq not found, defaulting QFI to 0\n";
        }

        const QfiContext* ctx = contextManager->getContextByQfi(qfi);

        // Lookup DRB mapping
        int drb = 0;

        if (ctx)
            drb = ctx->drbIndex;
        else
            EV_WARN << "SDAP TX: No mapping for QFI=" << (int)qfi << ", using default DRB 0\n";

        EV_INFO << "SDAP TX: Selected DRB=" << (int)drb << " for QFI=" << (int)qfi << "\n";

        // Build SDAP header
        auto sdapHeader = makeShared<NrSdapPdu>();
        sdapHeader->setQfi(qfi);
        sdapHeader->setD_c(true);
        sdapHeader->setReflectiveQoSIndicator(false);

        // extract the transport and network headers
        EV_INFO << "Before network: " << pkt->peekAtFront() << "\n";

        auto ipHeader = pkt->removeAtFront<inet::Ipv4Header>();

        int transportProtocol = ipHeader->getProtocolId();

        EV_INFO << "Before Transport: " << pkt->peekAtFront() << "\n";

        if (IP_PROT_TCP == transportProtocol) { // requires revision
            auto tcpHeader = pkt->removeAtFront<tcp::TcpHeader>();
            // Attach SDAP header
            pkt->insertAtFront(sdapHeader);
            //reinsert the the tcp header
            tcpHeader->setChunkLength(tcpHeader->getHeaderLength());
            pkt->insertAtFront(tcpHeader);

        }
        else if (IP_PROT_UDP == transportProtocol) {
            auto udpHeader = pkt->removeAtFront<inet::UdpHeader>();
            // Attach SDAP header
            pkt->insertAtFront(sdapHeader);
            //reinsert the the udp header
            udpHeader->setChunkLength(inet::UDP_HEADER_LENGTH);
            udpHeader->setTotalLengthField(udpHeader-> getTotalLengthField() + inet::B(sdapHeader->getChunkLength()));
            pkt->insertAtFront(udpHeader);
        }
        else {
            EV_WARN << "SDAP TX : unknown transport header";
        }

        EV_INFO << "After SDAP: " << pkt->peekAtFront() << "\n";
        EV_INFO << "SDAP TX: Inserted SDAP header with QFI = " << (int)qfi << "\n";

        ipHeader->setChunkLength(ipHeader->getHeaderLength());
        ipHeader->setTotalLengthField(ipHeader-> getTotalLengthField() + inet::B(sdapHeader->getChunkLength()));
        pkt->insertAtFront(ipHeader);
        EV_INFO << "After IP: " << pkt->peekAtFront() << "\n";
        EV_INFO << "DRB: " << drb << "\n";

        // this way sdap header is hidden behined the udp/ip headers, won't be compressed by pdcp.
        send(pkt, "stackSdap$o", drb); // route the packet to the proper drb.
    }
    else if (strcmp(arrivalGate->getBaseName(), "stackSdap") == 0) {
        EV_INFO << "Received packet from PDCP, forwarding to IP\n";
        // TODO: Strip SDAP header for reflective qos functionalities
        send(msg, "DataPort$o");
    }
    else {
        EV_ERROR << "Message arrived on unknown gate: " << arrivalGate->getFullName() << "\n";
        delete msg;
    }
}

} //namespace
