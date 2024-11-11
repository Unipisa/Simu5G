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

#ifndef _DEVAPPPACKETSERIALIZER_H_
#define _DEVAPPPACKETSERIALIZER_H_

#include <inet/common/packet/serializer/FieldsChunkSerializer.h>

namespace simu5g {

using namespace inet;

enum DevAppCode { START_MECAPP_CODE, STOP_MECAPP_CODE, START_ACK_CODE, STOP_ACK_CODE, START_NACK_CODE, STOP_NACK_CODE };

/**
 * Converts between ApplicationPacket and binary (network byte order) application packet.
 *
 * This serializer works for all the DeviceAppPacket packets.
 * The structure of a DeviceAppPacket packets is the following:
 * - packet type code (as of DevAppCode enum)
 * - length of the subsequent package size
 * - data
 *
 * auto pkt = pk->peekAtFront<DeviceAppPacket>();
 * if(strcmp(pkt->getType(), START_MECAPP) == 0)
 *      it is a DeviceAppStartPacket
 *
 * Codes:
 * 0 START
 * 1 STOP
 * 2 ACK START
 * 3 ACK STOP
 * 4 NACK START
 * 5 NACK STOP
 */

class DeviceAppMessageSerializer : public inet::FieldsChunkSerializer
{
  protected:
    void serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const override;
    const Ptr<Chunk> deserialize(MemoryInputStream& stream) const override;
};

} //namespace

#endif

