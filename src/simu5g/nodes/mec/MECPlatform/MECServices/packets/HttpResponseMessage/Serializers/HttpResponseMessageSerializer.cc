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

#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/Serializers/HttpResponseMessageSerializer.h"

#include <string>

#include <inet/common/packet/serializer/ChunkSerializerRegistry.h>

#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"

namespace simu5g {

using namespace inet;

Register_Serializer(HttpResponseMessage, HttpResponseMessageSerializer);

void HttpResponseMessageSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    EV << "HttpResponseMessageSerializer::serialize" << endl;
    auto startPosition = stream.getLength();
    const auto& applicationPacket = staticPtrCast<const HttpResponseMessage>(chunk);
    std::string payload = applicationPacket->getPayload();
    stream.writeBytes((const uint8_t *)payload.c_str(), B(payload.size()));
    int64_t remainders = B(applicationPacket->getChunkLength() - (stream.getLength() - startPosition)).get();
    if (remainders < 0)
        throw cRuntimeError("ApplicationPacket length = %d smaller than required %d bytes", (int)B(applicationPacket->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
    stream.writeByteRepeatedly('?', remainders);
}

// TODO
const Ptr<Chunk> HttpResponseMessageSerializer::deserialize(MemoryInputStream& stream) const
{
    EV << "HttpResponseMessageSerializer::deserialize" << endl;
    auto applicationPacket = makeShared<HttpResponseMessage>();
    return applicationPacket;
}

} //namespace

