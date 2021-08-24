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


#ifndef _WARNINGALERTPACKETSERIALIZER_H_
#define _WARNINGALERTPACKETSERIALIZER_H_

#include "inet/common/packet/serializer/FieldsChunkSerializer.h"

namespace inet {

/**
 * Converts between ApplicationPacket and binary (network byte order) application packet.
 *
 * This serializer works for all the WarningAppPacket packets.
 * The structure of a WarningAppPacket packets is the following:
 * - packet type code
 * - length of the subsequent package size
 * - data
 *
 * auto pkt = pk->peekAtFront<WarningAppPacket>();
 * if(strcmp(pkt->getType(), START_MECAPP) == 0)
 *      it is a WarningStartPacket
 *
 * Codes:
 * 0 START
 * 1 STOP
 * 2 ALERT
 */
class INET_API WarningAlertPacketSerializer : public FieldsChunkSerializer
{
  protected:
    virtual void serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const override;
    virtual const Ptr<Chunk> deserialize(MemoryInputStream& stream) const override;

  public:
    WarningAlertPacketSerializer() : FieldsChunkSerializer() {}
};

} // namespace inet

#endif

