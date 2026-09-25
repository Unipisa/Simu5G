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

#ifndef _CBRPACKETSERIALIZER_H_
#define _CBRPACKETSERIALIZER_H_

#include <inet/common/packet/serializer/FieldsChunkSerializer.h>

namespace simu5g {

/**
 * Converts between CbrPacket and binary (network byte order) packet: a 4-octet
 * length, then the fields the sender sets (frame count, frame id,
 * the timestamp as an 8-octet raw simulation time, payload size), then padding up
 * to the length, 24 octets at least. The arrival time is not on the wire; no
 * sender sets it.
 */
class CbrPacketSerializer : public inet::FieldsChunkSerializer
{
  protected:
    void serialize(inet::MemoryOutputStream& stream, const inet::Ptr<const inet::Chunk>& chunk) const override;
    const inet::Ptr<inet::Chunk> deserialize(inet::MemoryInputStream& stream) const override;
};

} //namespace

#endif
