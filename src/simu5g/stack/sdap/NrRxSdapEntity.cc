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

#include <map>
#include <string>
#include <sstream>

#include "simu5g/stack/sdap/packet/NrSdapPdu_m.h"
#include "simu5g/stack/sdap/common/QosTag_m.h"
#include "inet/common/packet/Packet.h"
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>

namespace simu5g {

Define_Module(NrRxSdapEntity);

// DRB mapping table (same as TX)
std::map<uint8_t, uint8_t> qfiToDrbMapRx;

void NrRxSdapEntity::initialize()
{
    // Load QFI-to-DRB mapping
    std::string mappingStr = par("qfiToDrbMapping").stdstringValue();

    std::stringstream ss(mappingStr);
    std::string pair;
    while (std::getline(ss, pair, ',')) {
        std::stringstream pairstream(pair);
        std::string qfiStr, drbStr;

        if (std::getline(pairstream, qfiStr, ':') && std::getline(pairstream, drbStr, ':')) {
            uint8_t qfi = std::stoi(qfiStr);
            uint8_t drb = std::stoi(drbStr);
            qfiToDrbMapRx[qfi] = drb;
            EV_INFO << "Loaded QFI->DRB mapping: QFI=" << (int)qfi << ", DRB=" << (int)drb << "\n";
        }
    }
}

void NrRxSdapEntity::handleMessage(cMessage *msg)
{
    auto arrivalGate = msg->getArrivalGate();
    auto pkt = check_and_cast<inet::Packet *>(msg);
    uint8_t qfi = 0;
    uint8_t drb = 0;

    if (arrivalGate == gate("DataPort$i")) {
        EV_INFO << "NrRxSdapEntity: Packet from IP → PDCP\n";
        // TODO: Add SDAP header or classification (if needed)
        send(msg, "stackSdap$o");
    }
    else if (arrivalGate == gate("stackSdap$i")) {
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

        // DRB lookup for consistency
        if (qfiToDrbMapRx.find(qfi) != qfiToDrbMapRx.end())
            drb = qfiToDrbMapRx[qfi];
        else
            EV_WARN << "No DRB mapping found for QFI=" << (int)qfi << ", using default DRB 0\n";

        EV_INFO << "SDAP RX: QFI=" << (int)qfi << ", mapped DRB=" << (int)drb << "\n";


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
