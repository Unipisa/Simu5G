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
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "apps/voip/VoipPacketSerializer.h"

#include <inet/common/packet/serializer/ChunkSerializerRegistry.h>

#include "apps/voip/VoipPacket_m.h"

namespace simu5g {

using namespace inet;

Register_Serializer(VoipPacket, VoipPacketSerializer);

void VoipPacketSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    auto startPosition = stream.getLength();
    const auto& voipPacket = staticPtrCast<const VoipPacket>(chunk);
    stream.writeUint32Be(B(voipPacket->getChunkLength()).get());
    stream.writeUint32Be(voipPacket->getIDtalk());
    stream.writeUint32Be(voipPacket->getNframes());
    stream.writeUint32Be(voipPacket->getIDframe());
    stream.writeUint64Be(voipPacket->getPayloadTimestamp().raw());
    stream.writeUint64Be(voipPacket->getArrivalTime().raw());
    stream.writeUint64Be(voipPacket->getPlayoutTime().raw());
    stream.writeUint32Be(voipPacket->getPayloadSize());

    int64_t remainders = B(voipPacket->getChunkLength() - (stream.getLength() - startPosition)).get();
    if (remainders < 0)
        throw cRuntimeError("VoipPacket length = %d smaller than required %d bytes", (int)B(voipPacket->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
    stream.writeByteRepeatedly('?', remainders);
}

const Ptr<Chunk> VoipPacketSerializer::deserialize(MemoryInputStream& stream) const
{
    auto startPosition = stream.getPosition();
    auto voipPacket = makeShared<VoipPacket>();
    B dataLength = B(stream.readUint32Be());
    voipPacket->setIDtalk(stream.readUint32Be());
    voipPacket->setNframes(stream.readUint32Be());
    voipPacket->setIDframe(stream.readUint32Be());
    voipPacket->getPayloadTimestamp().setRaw(stream.readUint64Be());
    voipPacket->getArrivalTime().setRaw(stream.readUint64Be());
    voipPacket->getPlayoutTime().setRaw(stream.readUint64Be());
    voipPacket->setPayloadSize(stream.readUint32Be());

    B remainders = dataLength - (stream.getPosition() - startPosition);
    ASSERT(remainders >= B(0));
    stream.readByteRepeatedly('?', B(remainders).get());
    return voipPacket;
}

} //namespace

