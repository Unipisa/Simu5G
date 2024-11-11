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

#include "x2/packet/LteX2MsgSerializer.h"

#include <inet/common/packet/serializer/ChunkSerializerRegistry.h>

#include "stack/compManager/X2CompMsg.h"
#include "stack/compManager/compManagerProportional/X2CompProportionalRequestIE.h"
#include "stack/compManager/compManagerProportional/X2CompProportionalReplyIE.h"
#include "stack/handoverManager/X2HandoverCommandIE.h"
#include "stack/handoverManager/X2HandoverControlMsg.h"
#include "x2/packet/LteX2Message.h"

namespace simu5g {

using namespace inet;

Register_Serializer(LteX2Message, LteX2MsgSerializer);

/**
 * This serializer performs serialization and deserialization for the X2 messages. This is needed
 * to send these messages over SCTP
 * Supported types:
 *
 * X2_COMP_MSG  (class X2CompMsg)
 * X2_HANDOVER_CONTROL_MSG  (class X2HandoverControlMsg)
 */

void LteX2MsgSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    auto startPosition = stream.getLength();
    const auto& msg = staticPtrCast<const LteX2Message>(chunk);
    LteX2MessageType type = msg->getType();
    if (type != X2_COMP_MSG && type != X2_HANDOVER_CONTROL_MSG)
        throw cRuntimeError("LteX2MsgSerializer::serialize of X2 message type is not implemented!");

    stream.writeByte(type);
    stream.writeUint32Be(num(msg->getSourceId()));
    stream.writeUint32Be(num(msg->getDestinationId()));
    // note: length does not need to be serialized - it is calculated during deserialization

    // serialization of list containing the information elements
    X2InformationElementsList ieList = msg->getIeList();
    stream.writeUint16Be(ieList.size());
    for (auto ie : ieList) {
        stream.writeByte(ie->getType());

        switch (ie->getType()) {
            case COMP_REQUEST_IE:
            case COMP_REPLY_IE:
                // no extra fields need to be serialized
                break;

            case COMP_PROP_REQUEST_IE: {
                X2CompProportionalRequestIE *propRequest = check_and_cast<X2CompProportionalRequestIE *>(ie);
                stream.writeUint32Be(propRequest->getNumBlocks());
                break;
            }
            case COMP_PROP_REPLY_IE: {
                X2CompProportionalReplyIE *propReply = check_and_cast<X2CompProportionalReplyIE *>(ie);
                serializeStatusMap(stream, propReply->getAllowedBlocksMap());
                break;
            }
            case X2_HANDOVER_CMD_IE: {
                X2HandoverCommandIE *handoverCmd = check_and_cast<X2HandoverCommandIE *>(ie);
                stream.writeByte(handoverCmd->isStartHandover());
                stream.writeUint16Be(num(handoverCmd->getUeId()));
                break;
            }
            default:
                throw cRuntimeError("LteX2MsgSerializer::serialize of this X2InformationElement not implemented!");
        }
    }

    int64_t remainders = B(msg->getChunkLength() - (stream.getLength() - startPosition)).get();
    if (remainders < 0)
        throw cRuntimeError("LteX2Msg length = %d smaller than required %d bytes", (int)B(msg->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
    else if (remainders > 0) {
        throw cRuntimeError("LteX2Msg length = %d larger than serialized %d bytes", (int)B(msg->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
    }
}

void LteX2MsgSerializer::serializeStatusMap(inet::MemoryOutputStream& stream, std::vector<CompRbStatus> map) const {
    stream.writeUint32Be(map.size());

    for (auto status : map) {
        stream.writeByte(status);
    }
}

const Ptr<Chunk> LteX2MsgSerializer::deserialize(MemoryInputStream& stream) const
{
    LteX2MessageType type = (LteX2MessageType)stream.readByte();
    Ptr<LteX2Message> msg;
    switch (type) {
        case X2_COMP_MSG:
            msg = makeShared<X2CompMsg>();
            break;
        case X2_HANDOVER_CONTROL_MSG:
            msg = makeShared<X2HandoverControlMsg>();
            break;
        default:
            throw cRuntimeError("LteX2MsgSerializer::deserialize of X2 message type is not implemented!");
    }

    msg->setSourceId(MacNodeId(stream.readUint32Be()));
    msg->setDestinationId(MacNodeId(stream.readUint32Be()));
    int32_t nrElements = stream.readUint16Be();
    for (int32_t i = 0; i < nrElements; i++) {
        X2InformationElement *ie;
        X2InformationElementType ieType = (X2InformationElementType)stream.readByte();

        switch (ieType) {
            case COMP_REQUEST_IE:
            case COMP_REPLY_IE:
                ie = new X2InformationElement(ieType);
                // no extra fields need to be deserialized
                break;
            case COMP_PROP_REQUEST_IE: {
                auto propRequest = new X2CompProportionalRequestIE();
                propRequest->setNumBlocks(stream.readUint32Be());
                ie = propRequest;
                break;
            }
            case COMP_PROP_REPLY_IE: {
                auto propReply = new X2CompProportionalReplyIE();
                std::vector<CompRbStatus> map = deserializeStatusMap(stream);
                propReply->setAllowedBlocksMap(map);
                ie = propReply;
                break;
            }
            case X2_HANDOVER_CMD_IE: {
                auto handoverCmd = new X2HandoverCommandIE();
                if (stream.readByte() != 0)
                    handoverCmd->setStartHandover();
                handoverCmd->setUeId(MacNodeId(stream.readUint16Be()));
                ie = handoverCmd;
                break;
            }
            default:
                throw cRuntimeError("LteX2MsgSerializer::serialize for X2InformationElement type not implemented!");
        }
        msg->pushIe(ie);
    }

    return msg;
}

std::vector<CompRbStatus> LteX2MsgSerializer::deserializeStatusMap(inet::MemoryInputStream& stream) const {
    uint32_t size = stream.readUint32Be();
    std::vector<CompRbStatus> map(size);
    for (uint32_t i = 0; i < size; i++)
        map[i] = (CompRbStatus)stream.readByte();
    return map;
}

} //namespace

