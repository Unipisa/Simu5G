//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/corenetwork/gtp/GtpUserMsgSerializer.h"

#include <inet/common/packet/serializer/ChunkSerializerRegistry.h>

#include "simu5g/corenetwork/gtp/GtpUserMsg_m.h"

namespace simu5g {

using namespace inet;

Register_Serializer(GtpUserMsg, GtpUserMsgSerializer);

// GTP-U header (TS 29.281 5.1): flags (version 1, protocol type GTP, E/S/PN), message
// type, length, TEID; when E is set, the optional sequence number, N-PDU number and next
// extension header type, followed by the extension headers
static const uint8_t GTPU_FLAGS = 0x30;                     // version 1, protocol type GTP
static const uint8_t GTPU_FLAG_E = 0x04;                    // an extension header follows
static const uint8_t GTPU_OPTIONAL_FIELDS = 0x07;           // E, S or PN: the optional octets are present
static const uint8_t EXT_PDU_SESSION_CONTAINER = 0x85;      // TS 29.281 5.2.1
static const uint8_t EXT_NONE = 0x00;

void GtpUserMsgSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    const auto& gtpUserMsg = staticPtrCast<const GtpUserMsg>(chunk);
    PduSessionContainerType container = gtpUserMsg->getPduSessionContainer();
    stream.writeByte(GTPU_FLAGS | (container != PDU_SESSION_CONTAINER_NONE ? GTPU_FLAG_E : 0));
    stream.writeByte(gtpUserMsg->getMessageType());
    stream.writeUint16Be(gtpUserMsg->getLengthField());
    stream.writeUint32Be(num(gtpUserMsg->getTeid()));
    if (container != PDU_SESSION_CONTAINER_NONE) {
        stream.writeUint16Be(0);   // sequence number, not used
        stream.writeByte(0);       // N-PDU number, not used
        stream.writeByte(EXT_PDU_SESSION_CONTAINER);
        // PDU Session Container (TS 38.415 5.5.2): its length in 4-octet units, the PDU
        // Type and the QFI of the PDU SESSION INFORMATION, and the next extension type
        stream.writeByte(1);
        stream.writeByte(container << 4);
        stream.writeByte(num(gtpUserMsg->getQfi()) & 0x3f);
        stream.writeByte(EXT_NONE);
    }
}

const Ptr<Chunk> GtpUserMsgSerializer::deserialize(MemoryInputStream& stream) const
{
    auto startPosition = stream.getPosition();
    auto gtpUserMsg = makeShared<GtpUserMsg>();
    uint8_t flags = stream.readByte();
    if ((flags & 0xf0) != GTPU_FLAGS)
        gtpUserMsg->markIncorrect();
    gtpUserMsg->setMessageType(stream.readByte());
    gtpUserMsg->setLengthField(stream.readUint16Be());
    gtpUserMsg->setTeid(Teid(stream.readUint32Be()));
    gtpUserMsg->setQfi(Qfi(0));   // the default flow, unless a PDU Session Container names another
    if (flags & GTPU_OPTIONAL_FIELDS) {
        stream.readUint16Be();     // sequence number
        stream.readByte();         // N-PDU number
        uint8_t nextType = stream.readByte();
        while (nextType != EXT_NONE && !gtpUserMsg->isIncorrect()) {
            B extensionLength = B(4 * stream.readByte());   // including its length and next-type octets
            if (extensionLength < B(4))
                gtpUserMsg->markIncorrect();
            else if (nextType == EXT_PDU_SESSION_CONTAINER) {
                gtpUserMsg->setPduSessionContainer((PduSessionContainerType)(stream.readByte() >> 4));
                gtpUserMsg->setQfi(Qfi(stream.readByte() & 0x3f));
                stream.seek(stream.getPosition() + extensionLength - B(4));   // the rest of the PDU SESSION INFORMATION
            }
            else
                stream.seek(stream.getPosition() + extensionLength - B(2));   // an extension header not modeled
            nextType = stream.readByte();
        }
    }
    gtpUserMsg->setChunkLength(stream.getPosition() - startPosition);
    return gtpUserMsg;
}

} //namespace

