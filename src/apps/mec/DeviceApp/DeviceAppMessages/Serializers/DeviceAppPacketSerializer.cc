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

#include "../../../../../apps/mec/DeviceApp/DeviceAppMessages/Serializers/DeviceAppPacketSerializer.h"

#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include <string>

#include "../../../../../apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_m.h"
#include "../../../../../apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
namespace inet {

Register_Serializer(DeviceAppPacket, DeviceAppMessageSerializer);
Register_Serializer(DeviceAppStartAckPacket, DeviceAppMessageSerializer);
Register_Serializer(DeviceAppStartPacket, DeviceAppMessageSerializer);
Register_Serializer(DeviceAppStopAckPacket, DeviceAppMessageSerializer);
Register_Serializer(DeviceAppStopPacket, DeviceAppMessageSerializer);


void DeviceAppMessageSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    EV << "DeviceAppMessageSerializer::serialize" << endl;

    auto startPosition = stream.getLength();
    auto devAppPacket = staticPtrCast<const DeviceAppPacket>(chunk);
    std::string  ss = devAppPacket->getType();
//
    if(strcmp(ss.c_str(), START_MECAPP) == 0)
    {
        auto startPk = staticPtrCast<const DeviceAppStartPacket>(chunk);
        stream.writeByte((uint8_t) START_MECAPP_CODE);
        std::stringstream data;
        data << startPk->getMecAppName();
        std::string sdata = data.str();
        stream.writeByte((uint8_t)(sdata.size()));
        stream.writeString(sdata);
        int64_t remainders = B(startPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("DeviceApp - START_MECAPP_CODE length = %d smaller than required %d bytes", (int)B(startPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else if(strcmp(ss.c_str(), ACK_START_MECAPP) == 0)
    {
        auto ackPk = staticPtrCast<const DeviceAppStartAckPacket>(chunk);

        if(ackPk->getResult() == true)
        {
            std::stringstream data;
            data << ackPk->getIpAddress() << ":" << ackPk->getPort();
            std::string sdata = data.str();
            stream.writeByte((uint8_t) START_ACK_CODE);
            stream.writeByte((uint8_t)(sdata.size()));
            stream.writeString(sdata);
            int64_t remainders = B(ackPk->getChunkLength() - (stream.getLength() - startPosition)).get();
            if (remainders < 0)
                throw cRuntimeError("DeviceApp - START_ACK_CODE length = %d smaller than required %d bytes. Data: %s [%d]",
                        (int)B(ackPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get(), sdata.c_str(), sdata.size());
            stream.writeByteRepeatedly('?', remainders);
        }
        else
        {
            stream.writeByte((uint8_t) START_NACK_CODE);
            stream.writeByte((uint8_t)0);

            int64_t remainders = B(ackPk->getChunkLength() - (stream.getLength() - startPosition)).get();
            if (remainders < 0)
                throw cRuntimeError("DeviceApp - START_NACK_CODE length = %d smaller than required %d bytes", (int)B(ackPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
            stream.writeByteRepeatedly('?', remainders);
        }
    }
    else if(strcmp(ss.c_str(), STOP_MECAPP) == 0)
    {
        auto stopPk = staticPtrCast<const DeviceAppStopPacket>(chunk);
        stream.writeByte((uint8_t) STOP_MECAPP_CODE);
        std::stringstream data;
        data << stopPk->getContextId();
        std::string sdata = data.str();
        stream.writeByte((uint8_t)(sdata.size()));
        stream.writeString(sdata);
        int64_t remainders = B(stopPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("DeviceApp - STOP_MECAPP_CODE length = %d smaller than required %d bytes", (int)B(stopPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else if(strcmp(ss.c_str(), ACK_STOP_MECAPP) == 0)
    {
        auto ackPk = staticPtrCast<const DeviceAppStopAckPacket>(chunk);
        if(ackPk->getResult() == true)
        {
            stream.writeByte((uint8_t) STOP_ACK_CODE);
            stream.writeByte((uint8_t) 0);

            int64_t remainders = B(ackPk->getChunkLength() - (stream.getLength() - startPosition)).get();
            if (remainders < 0)
                throw cRuntimeError("DeviceApp - STOP_ACK_CODE length = %d smaller than required %d bytes", (int)B(ackPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
            stream.writeByteRepeatedly('?', remainders);
        }
        else
        {
            stream.writeByte((uint8_t) STOP_NACK_CODE);
            stream.writeByte((uint8_t) 0);
            int64_t remainders = B(ackPk->getChunkLength() - (stream.getLength() - startPosition)).get();
            if (remainders < 0)
                throw cRuntimeError("DeviceApp - STOP_NACK_CODE length = %d smaller than required %d bytes", (int)B(ackPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
            stream.writeByteRepeatedly('?', remainders);
        }
    }
    else
    {
        throw cRuntimeError("DeviceAppMessageSerializer::serialize - Code %d not recognized!", ss.c_str());
    }
}

const Ptr<Chunk> DeviceAppMessageSerializer::deserialize(MemoryInputStream& stream) const
{

    EV << "DeviceAppMessageSerializer::deserialize" << endl;

    int messageCode = (int)(stream.readByte());
    auto code = static_cast<DevAppCode>(messageCode);

    B messageDataLength = B(stream.readByte());

    std::vector<uint8_t> bytes;

    switch (code)
    {
        case START_MECAPP_CODE:
        {
            auto startPacket = makeShared<DeviceAppStartPacket>();
            startPacket->setType(START_MECAPP);
            stream.readBytes(bytes, messageDataLength);
            startPacket->setMecAppName((char*)&bytes[0]);
            return startPacket;
            break;
        }
        case STOP_MECAPP_CODE:
        {
            auto stopPacket = makeShared<DeviceAppStopPacket>();
            stopPacket->setType(STOP_MECAPP);
            stream.readBytes(bytes, messageDataLength);
            stopPacket->setContextId((char*)&bytes[0]);
            // put timestamp tag
            return stopPacket;
            break;
        }
        case START_ACK_CODE:
        {
            auto ackPacket = makeShared<DeviceAppStartAckPacket>();
            ackPacket->setType(ACK_START_MECAPP);
            ackPacket->setResult("ACK");
            stream.readBytes(bytes, messageDataLength);  // endPoint ipAddress:port (for now) or something else (later)
//            std::string endPoint(bytes.begin(), bytes.end());
            std::vector<std::string> ipPort = cStringTokenizer((char*)&bytes[0], ":").asVector();
            if(ipPort.size() != 2)
                throw cRuntimeError("DeviceAppMessageSerializer::deserialize - endPoint must be ip:port, but is: %s ", (char*)&bytes[0]);
            ackPacket->setIpAddress(ipPort[0].c_str());
            ackPacket->setPort(atoi(ipPort[1].c_str()));
            return ackPacket;
            break;
        }
        case START_NACK_CODE:
        {
            auto stopAckPacket = makeShared<DeviceAppStartAckPacket>();
            stopAckPacket->setType(ACK_START_MECAPP);
            stopAckPacket->setResult("NACK");
            // put timestamp tag
            return stopAckPacket;
            break;
        }
        case STOP_ACK_CODE:
        {
            auto stopAckPacket = makeShared<DeviceAppStopAckPacket>();
            stopAckPacket->setType(ACK_STOP_MECAPP);
            stopAckPacket->setResult("ACK");
            // put timestamp tag
            return stopAckPacket;
            break;
        }
        case STOP_NACK_CODE:
        {
            auto stopAckPacket = makeShared<DeviceAppStopAckPacket>();
            stopAckPacket->setType(ACK_STOP_MECAPP);
            stopAckPacket->setResult("NACK");
            // put timestamp tag
            return stopAckPacket;
            break;
        }
        default:
        {
            throw cRuntimeError("DeviceAppMessageSerializer::deserialize - Code %d not recognized!", messageCode);
        }
    }

    throw cRuntimeError("DeviceAppMessageSerializer::deserialize - Cancella codice sotto!");

}

} // namespace inet

