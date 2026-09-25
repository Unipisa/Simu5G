//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/apps/burst/BurstPacketSerializer.h"

#include <inet/common/packet/serializer/ChunkSerializerRegistry.h>

#include "simu5g/apps/burst/BurstPacket_m.h"

namespace simu5g {

using namespace inet;

Register_Serializer(BurstPacket, BurstPacketSerializer);

void BurstPacketSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    auto startPosition = stream.getLength();
    const auto& burstPacket = staticPtrCast<const BurstPacket>(chunk);
    stream.writeUint32Be(B(burstPacket->getChunkLength()).get());
    stream.writeUint32Be(burstPacket->getMsgId());
    stream.writeUint64Be(burstPacket->getPayloadTimestamp().raw());
    stream.writeUint32Be(burstPacket->getPayloadSize());

    int64_t remainders = B(burstPacket->getChunkLength() - (stream.getLength() - startPosition)).get();
    if (remainders < 0)
        throw cRuntimeError("BurstPacket length = %d smaller than required %d bytes", (int)B(burstPacket->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
    stream.writeByteRepeatedly('?', remainders);
}

const Ptr<Chunk> BurstPacketSerializer::deserialize(MemoryInputStream& stream) const
{
    auto startPosition = stream.getPosition();
    auto burstPacket = makeShared<BurstPacket>();
    B dataLength = B(stream.readUint32Be());
    burstPacket->setMsgId(stream.readUint32Be());
    burstPacket->setPayloadTimestamp(SimTime::fromRaw(stream.readUint64Be()));
    burstPacket->setPayloadSize(stream.readUint32Be());

    B remainders = dataLength - (stream.getPosition() - startPosition);
    ASSERT(remainders >= B(0));
    stream.readByteRepeatedly('?', B(remainders).get());
    return burstPacket;
}

} //namespace
