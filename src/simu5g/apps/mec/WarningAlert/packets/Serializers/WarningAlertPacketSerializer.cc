//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "apps/mec/WarningAlert/packets/Serializers/WarningAlertPacketSerializer.h"

#include <string>

#include <inet/common/packet/serializer/ChunkSerializerRegistry.h>

#include "apps/mec/WarningAlert/packets/WarningAlertPacket_m.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_Types.h"

namespace simu5g {

using namespace inet;

Register_Serializer(WarningAppPacket, WarningAlertPacketSerializer);
Register_Serializer(WarningStartPacket, WarningAlertPacketSerializer);
Register_Serializer(WarningAlertPacket, WarningAlertPacketSerializer);

void WarningAlertPacketSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    EV << "WarningAlertPacketSerializer::serialize" << endl;

    auto startPosition = stream.getLength();
    auto devAppPacket = staticPtrCast<const WarningAppPacket>(chunk);
    std::string ss = devAppPacket->getType();

    if (ss == START_WARNING) {
        auto startPk = staticPtrCast<const WarningStartPacket>(chunk);
        stream.writeByte((uint8_t)0);

        double xCoord = startPk->getCenterPositionX();
        double yCoord = startPk->getCenterPositionY();
        double radius = startPk->getRadius();

        std::stringstream coords;
        coords << xCoord << "," << yCoord << "," << radius;
        std::string scoords = coords.str();
        stream.writeByte((uint8_t)scoords.size());
        stream.writeString(scoords);

        int64_t remainders = B(startPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(startPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else if (ss == STOP_WARNING) {
        auto stopPk = staticPtrCast<const WarningStopPacket>(chunk);
        stream.writeByte((uint8_t)1); // CODE
        stream.writeByte((uint8_t)0); // Length

        int64_t remainders = B(stopPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(stopPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else if (ss == WARNING_ALERT) {
        auto alertPk = staticPtrCast<const WarningAlertPacket>(chunk);
        stream.writeByte((uint8_t)2);

        double xCoord = alertPk->getPositionX();
        double yCoord = alertPk->getPositionY();

        std::stringstream coords;
        coords << xCoord << "," << yCoord;
        std::string scoords = coords.str();
        stream.writeByte((uint8_t)scoords.size() + 1);
        stream.writeByte((uint8_t)alertPk->getDanger());
        stream.writeString(scoords);

        int64_t remainders = B(alertPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(alertPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else if (ss == START_ACK) {
        auto alertPk = staticPtrCast<const WarningAlertPacket>(chunk);
        stream.writeByte((uint8_t)3);
        int64_t remainders = B(alertPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(alertPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else if (ss == START_NACK) {
        auto alertPk = staticPtrCast<const WarningAlertPacket>(chunk);
        stream.writeByte((uint8_t)4);
        int64_t remainders = B(alertPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(alertPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else {
        throw cRuntimeError("WarningAlertPacketSerializer - WarningAlertPacket type %s not recognized", ss.c_str());
    }
}

const Ptr<Chunk> WarningAlertPacketSerializer::deserialize(MemoryInputStream& stream) const
{
    EV << "WarningAlertPacketSerializer::deserialize" << endl;

    B totalLength = stream.getLength();

    int messageCode = (int)(stream.readByte());

    std::vector<uint8_t> bytes;

    switch (messageCode) {
        case 0: // START
        {
            auto startPacket = makeShared<WarningStartPacket>();

            startPacket->setType(START_WARNING);
            B messageDataLength = B(stream.readByte());
            std::vector<uint8_t> bytes;
            stream.readBytes(bytes, messageDataLength);
            std::string data(bytes.begin(), bytes.end());

            std::vector<std::string> coords = cStringTokenizer((char *)&bytes[0], ",").asVector();
            if (coords.size() != 3)
                throw cRuntimeError("WarningAlertPacketSerializer::deserialize - start application message must be in the form x,y,radius. But is %s", data.c_str());

            double x = std::stod(coords[0]);
            double y = std::stod(coords[1]);
            double c = std::stod(coords[2]);

            startPacket->setCenterPositionX(x);
            startPacket->setCenterPositionY(y);
            startPacket->setRadius(c);

            return startPacket;
        }
        case 1: // STOP
        {
            auto stopPacket = makeShared<WarningStopPacket>();
            stopPacket->setType(STOP_WARNING);
            return stopPacket;
        }
        case 2: // ALERT
        {
            auto alertPacket = makeShared<WarningAlertPacket>();
            alertPacket->setType(WARNING_ALERT);

            B messageDataLength = B(stream.readByte());
            bool res = (bool)(stream.readByte());
            std::vector<uint8_t> bytes;
            stream.readBytes(bytes, messageDataLength);
            std::string data(bytes.begin(), bytes.end());

            std::vector<std::string> coords = cStringTokenizer((char *)&bytes[0], ",").asVector();

            if (coords.size() != 2)
                throw cRuntimeError("WarningAlertPacketSerializer::deserialize - WARNING application message must be in the form x,y. But is %s", data.c_str());
            alertPacket->setDanger(res);
            alertPacket->setPositionX(std::stod(coords[0]));
            alertPacket->setPositionY(std::stod(coords[1]));

            B remainders = totalLength - (stream.getPosition());
            ASSERT(remainders >= B(0));
            stream.readByteRepeatedly('?', remainders.get());

            return alertPacket;

            break;
        }

        case 3: // ACK START
        {
            auto ackStartPacket = makeShared<WarningAppPacket>();
            ackStartPacket->setType(START_ACK);

            B remainders = totalLength - (stream.getPosition());
            ASSERT(remainders >= B(0));
            stream.readByteRepeatedly('?', remainders.get());

            return ackStartPacket;
        }
        case 4: // NACK START
        {
            auto nackStartPacket = makeShared<WarningAppPacket>();
            nackStartPacket->setType(START_NACK);

            B remainders = totalLength - (stream.getPosition());
            ASSERT(remainders >= B(0));
            stream.readByteRepeatedly('?', remainders.get());

            return nackStartPacket;
        }

        default: {
            throw cRuntimeError("WarningAlertPacketSerializer::deserialize - Code %d not recognized!", messageCode);
        }
    }
}

} //namespace

