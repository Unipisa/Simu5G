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

#include "simu5g/apps/cbr/CbrPacketSerializer.h"

#include <inet/common/packet/serializer/ChunkSerializerRegistry.h>

#include "simu5g/apps/cbr/CbrPacket_m.h"

namespace simu5g {

using namespace inet;

Register_Serializer(CbrPacket, CbrPacketSerializer);

void CbrPacketSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    auto startPosition = stream.getLength();
    const auto& cbrPacket = staticPtrCast<const CbrPacket>(chunk);
    stream.writeUint32Be(B(cbrPacket->getChunkLength()).get());
    stream.writeUint32Be(cbrPacket->getNumFrames());
    stream.writeUint32Be(cbrPacket->getFrameId());
    stream.writeUint64Be(cbrPacket->getPayloadTimestamp().raw());
    stream.writeUint32Be(cbrPacket->getPayloadSize());

    int64_t remainders = B(cbrPacket->getChunkLength() - (stream.getLength() - startPosition)).get();
    if (remainders < 0)
        throw cRuntimeError("CbrPacket length = %d smaller than required %d bytes", (int)B(cbrPacket->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
    stream.writeByteRepeatedly('?', remainders);
}

const Ptr<Chunk> CbrPacketSerializer::deserialize(MemoryInputStream& stream) const
{
    auto startPosition = stream.getPosition();
    auto cbrPacket = makeShared<CbrPacket>();
    B dataLength = B(stream.readUint32Be());
    cbrPacket->setNumFrames(stream.readUint32Be());
    cbrPacket->setFrameId(stream.readUint32Be());
    cbrPacket->setPayloadTimestamp(SimTime::fromRaw(stream.readUint64Be()));
    cbrPacket->setPayloadSize(stream.readUint32Be());

    B remainders = dataLength - (stream.getPosition() - startPosition);
    ASSERT(remainders >= B(0));
    stream.readByteRepeatedly('?', B(remainders).get());
    return cbrPacket;
}

} //namespace
