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

#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessageSerializer.h"

#include "nodes/mec/MEPlatform/MeServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"

#include <string>
namespace inet {

Register_Serializer(HttpResponseMessage, HttpResponseMessageSerializer);

void HttpResponseMessageSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    EV << "HttpResponseMessageSerializer::serialize" << endl;
    auto startPosition = stream.getLength();
    const auto& applicationPacket = staticPtrCast<const HttpResponseMessage>(chunk);
    std::string payload = applicationPacket->getPayload();
    stream.writeBytes((const uint8_t*)payload.c_str(), B(payload.size()));
    EV << "bytes " << B(stream.getLength());

//    EV << "stream " << payload << endl;
//    stream.writeUint32Be(B(applicationPacket->getChunkLength()).get());
//    stream.writeUint32Be(applicationPacket->getSequenceNumber());
    int64_t remainders = B(applicationPacket->getChunkLength() - (stream.getLength() - startPosition)).get();
    if (remainders < 0)
        throw cRuntimeError("ApplicationPacket length = %d smaller than required %d bytes", (int)B(applicationPacket->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
    stream.writeByteRepeatedly('?', remainders);
}

const Ptr<Chunk> HttpResponseMessageSerializer::deserialize(MemoryInputStream& stream) const
{
    EV << "HttpResponseMessageSerializer::deserialize" << endl;
    auto startPosition = stream.getPosition();
    auto applicationPacket = makeShared<HttpResponseMessage>();
    B dataLength = B(stream.getLength());
    size_t pos = 0;
    //
//    parse HTTP packet

        std::vector<uint8_t> bytes = stream.getData();
//        stream.readBytes(bytes, B(150));
//
        std::string packet(bytes.begin(), bytes.end());
//
//
        EV << "sddd" << packet << endl;
//        return nullptr;
//        header = packet.substr(0, pos);
//        packet.erase(0, pos+delimiter.length()); //remove header
//        HttpResponseMessage* response = Http::parseHeader(header);
////        response->setBody(packet.c_)
//        Http::HttpMsgState res = Http::parseTcpData(&packet, respnse);
//
//
//    }
//
//    B remainders = stream.getLength() - (stream.getPosition() - startPosition);
//    ASSERT(remainders >= B(0));
//    EV << "data remainders " << remainders<<endl;
//    stream.readByteRepeatedly('?', B(remainders).get());
//    return applicationPacket;
        EV << "HttpResponseMessageSerializer::deserialize " << dataLength.get() << endl;
        return applicationPacket;
}

} // namespace inet

