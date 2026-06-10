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

#include "X2HandoverDataMsgSerializer.h"
#include "simu5g/x2/packet/LteX2Message.h"
#include "simu5g/x2/packet/LteX2Message_m.h"
#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"
#include "simu5g/stack/handoverManager/X2HandoverDataMsg.h"


// class added to allow proper serialization of handover data
using namespace inet;
using namespace simu5g;
Register_Serializer(X2HandoverDataMsg, X2HandoverDataMsgSerializer);

void X2HandoverDataMsgSerializer::serialize(inet::MemoryOutputStream &stream,
        const inet::Ptr<const inet::Chunk> &chunk) const {
       auto startPosition = stream.getLength();

        const auto& x2DataMsg = staticPtrCast<const X2HandoverDataMsg>(chunk);

        //We need to write to the stream exactly getChunkLength(), which is 11 for ltex2messages. If we write only 8,  then fill with remainders
        //We cannot write the length because we write 12 bytes and cannot change the ltex2message length

        stream.writeUint24Be(B(x2DataMsg->getChunkLength()).get());

        stream.writeUint32Be(num(x2DataMsg->getDestinationId()));
        stream.writeUint32Be(num(x2DataMsg->getSourceId()));

        int64_t remainders = B(x2DataMsg->getChunkLength() - (stream.getLength() - startPosition)).get();
        std::cout<<simTime()<<"X2HandoverDataMsgSerializer::serialize: startPosition="<<startPosition<<" chunkLength="<<x2DataMsg->getChunkLength()<<" stream.getLength()="<<stream.getLength()<<" remainders="<<remainders<<endl;
        if (remainders < 0) {
            throw cRuntimeError("X2HandoverDataMsgSerializer length = %d smaller than required %d bytes", (int)B(x2DataMsg->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        } else {
            stream.writeByteRepeatedly('?', remainders);
        }
}

const inet::Ptr<inet::Chunk> X2HandoverDataMsgSerializer::deserialize(
        inet::MemoryInputStream &stream) const {
       std::cout <<"X2HandoverDataMsgSerializer::deserialize "<<endl;
       auto startPosition = stream.getPosition();
       auto x2DataMsg = makeShared<X2HandoverDataMsg>();
       B dataLength = B(stream.readUint24Be());

       x2DataMsg->setDestinationId(MacNodeId(stream.readUint32Be()));
       x2DataMsg->setSourceId(MacNodeId(stream.readUint32Be()));

       B remainders = dataLength - (stream.getPosition() - startPosition);
       //ASSERT(remainders >= B(0));
       if (remainders.get()>=0) {
           stream.readByteRepeatedly('?', B(remainders).get());
       }
       return x2DataMsg;
}
