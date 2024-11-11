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

#include "corenetwork/gtp/GtpUserMsgSerializer.h"

#include <inet/common/packet/serializer/ChunkSerializerRegistry.h>

#include "corenetwork/gtp/GtpUserMsg_m.h"

namespace simu5g {

using namespace inet;

Register_Serializer(GtpUserMsg, GtpUserMsgSerializer);

void GtpUserMsgSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    auto startPosition = stream.getLength();
    const auto& gtpUserMsg = staticPtrCast<const GtpUserMsg>(chunk);
    stream.writeUint32Be(B(gtpUserMsg->getChunkLength()).get());
    stream.writeUint32Be(gtpUserMsg->getTeid());

    int64_t remainders = B(gtpUserMsg->getChunkLength() - (stream.getLength() - startPosition)).get();
    if (remainders < 0)
        throw cRuntimeError("gtpUserMsg length = %d smaller than required %d bytes", (int)B(gtpUserMsg->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
    stream.writeByteRepeatedly('?', remainders);
}

const Ptr<Chunk> GtpUserMsgSerializer::deserialize(MemoryInputStream& stream) const
{
    auto startPosition = stream.getPosition();
    auto gtpUserMsg = makeShared<GtpUserMsg>();
    B dataLength = B(stream.readUint32Be());
    gtpUserMsg->setTeid(stream.readUint32Be());

    B remainders = dataLength - (stream.getPosition() - startPosition);
    ASSERT(remainders >= B(0));
    stream.readByteRepeatedly('?', B(remainders).get());
    return gtpUserMsg;
}

} //namespace

