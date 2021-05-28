//
// Copyright (C) 2020 OpenSim Ltd.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "apps/mec/warningAlert_rest/packets/Serializers/WarningAlertPacketSerializer.h"

#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include <string>

#include "apps/mec/warningAlert_rest/packets/WarningAlertPacket_m.h"
#include "apps/mec/warningAlert_rest/packets/WarningAlertPacket_Types.h"

#include <iostream> // used to debug in emulation

namespace inet {

Register_Serializer(WarningAppPacket, WarningAlertPacketSerializer);
Register_Serializer(WarningStartPacket, WarningAlertPacketSerializer);
Register_Serializer(WarningAlertPacket, WarningAlertPacketSerializer);


void WarningAlertPacketSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    EV << "WarningAlertPacketSerializer::serialize" << endl;

    auto startPosition = stream.getLength();
    auto devAppPacket = staticPtrCast<const WarningAppPacket>(chunk);
    std::string  ss = devAppPacket->getType();
//
    if(strcmp(ss.c_str(), START_WARNING) == 0)
    {
        auto startPk = staticPtrCast<const WarningStartPacket>(chunk);
        stream.writeByte((uint8_t) 0);

        double xCoord = startPk->getCenterPositionX();
        double yCoord = startPk->getCenterPositionY();
        double radius = startPk->getRadius();

        std::stringstream coords;
        coords << xCoord <<"," << yCoord << ","<< radius;
        std::string scoords = coords.str();
        stream.writeByte((uint8_t) scoords.size());
        stream.writeString(scoords);

        int64_t remainders = B(startPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(startPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }

    else if(strcmp(ss.c_str(), STOP_WARNING) == 0)
    {
       // TODO implement it
    }
    else if(strcmp(ss.c_str(), WARNING_ALERT) == 0)
    {
        auto alertPk = staticPtrCast<const WarningAlertPacket>(chunk);
        stream.writeByte((uint8_t) 2);

        double xCoord = alertPk->getPositionX();
        double yCoord = alertPk->getPositionY();

        std::stringstream coords;
        coords <<xCoord <<"," << yCoord;
        std::string scoords = coords.str();
        stream.writeByte((uint8_t) scoords.size()+1);
        stream.writeByte((uint8_t) alertPk->getDanger());
        stream.writeString(scoords);

        int64_t remainders = B(alertPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(alertPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else
    {
        throw cRuntimeError("WarningAlertPacketSerializer - WarningAlertPacket type %s not recognized", ss.c_str());
    }

}

const Ptr<Chunk> WarningAlertPacketSerializer::deserialize(MemoryInputStream& stream) const
{
    EV << "WarningAlertPacketSerializer::deserialize" << endl;

    B totalLength = stream.getLength();

    int messageCode = (int)(stream.readByte());

    std::vector<uint8_t> bytes;

    switch (messageCode)
    {
        case 0: // START
        {
            auto startPacket = makeShared<WarningStartPacket>();

            startPacket->setType(START_WARNING);
            std::cout << "type " << startPacket->getType()<< endl;
            B messageDataLength = B(stream.readByte());
            std::vector<uint8_t> bytes;
            stream.readBytes(bytes, messageDataLength);
            std::string data(bytes.begin(), bytes.end());

            std::vector<std::string> coords = cStringTokenizer((char*)&bytes[0], ",").asVector();
            if(coords.size() != 3)
                throw cRuntimeError("DeviceAppMessageSerializer::deserialize - start application message must be in form x,y,radius. But is %s", data.c_str());

            double x = std::stod(coords[0]);
            double y = std::stod(coords[1]);
            double c = std::stod(coords[2]);

            startPacket->setCenterPositionX(x);
            startPacket->setCenterPositionY(y);
            startPacket->setRadius(c);

            return startPacket;
            break;
        }
        case 1:
        {
            throw cRuntimeError("DeviceAppMessageSerializer::deserialize - STOP application message is not implemented, yet");
            break;
        }
        case 2:
        {
            auto alertPacket = makeShared<WarningAlertPacket>();
            alertPacket->setType(WARNING_ALERT);
            bool res = (bool)(stream.readByte());
            B messageDataLength = B(stream.readByte());
            std::vector<uint8_t> bytes;
            stream.readBytes(bytes, messageDataLength);
            std::string data(bytes.begin(), bytes.end());

            std::vector<std::string> coords = cStringTokenizer((char*)&bytes[0], ",").asVector();

            if(coords.size() != 2)
                throw cRuntimeError("DeviceAppMessageSerializer::deserialize - WARNING application message must be in form x,y. But is %s", data.c_str());
            alertPacket->setDanger(res);
            alertPacket->setPositionX(std::stod(coords[0]));
            alertPacket->setPositionY(std::stod(coords[1]));

//            throw cRuntimeError("len %d, pos %d", B(stream.getLength()).get(),B(stream.getPosition()).get());
            B remainders = totalLength - (stream.getPosition());
            ASSERT(remainders >= B(0));
            stream.readByteRepeatedly('?', remainders.get());

            return alertPacket;

            break;
        }

        default:
        {
            throw cRuntimeError("DeviceAppMessageSerializer::deserialize - Code %d not recognized!", messageCode);
        }
    }

}

} // namespace inet

