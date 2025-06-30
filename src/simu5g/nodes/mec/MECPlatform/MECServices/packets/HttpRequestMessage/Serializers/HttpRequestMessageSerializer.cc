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

#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/Serializers/HttpRequestMessageSerializer.h"

#include <inet/common/packet/serializer/ChunkSerializerRegistry.h>

#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"

namespace simu5g {

using namespace inet;

Register_Serializer(HttpRequestMessage, HttpRequestMessageSerializer);

void HttpRequestMessageSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    EV << "HttpRequestMessageSerializer::serialize" << endl;
    auto startPosition = stream.getLength();
    const auto& applicationPacket = staticPtrCast<const HttpRequestMessage>(chunk);
    stream.writeBytes((const uint8_t *)applicationPacket->getPayload().c_str(), B(applicationPacket->getPayload().size()));

    int64_t remainders = B(applicationPacket->getChunkLength() - (stream.getLength() - startPosition)).get();
    if (remainders < 0)
        throw cRuntimeError("ApplicationPacket length = %d smaller than required %d bytes", (int)B(applicationPacket->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
    stream.writeByteRepeatedly('?', remainders);
}

const Ptr<Chunk> HttpRequestMessageSerializer::deserialize(MemoryInputStream& stream) const
{
    EV << "HttpRequestMessageSerializer::deserialize" << endl;
    auto startPosition = stream.getPosition();
    auto applicationPacket = makeShared<HttpRequestMessage>();
    std::string data = stream.readString();
    applicationPacket = check_and_cast<HttpRequestMessage *>(Http::parseHeader(data));

    B remainders = stream.getLength() - (stream.getPosition() - startPosition);
    ASSERT(remainders >= B(0));
    stream.readByteRepeatedly('?', B(remainders).get());
    return applicationPacket;
}

} //namespace

