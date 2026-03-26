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

#ifndef __INET_GTPUSERMSGSERIALIZER_H
#define __INET_GTPUSERMSGSERIALIZER_H

#include <inet/common/packet/serializer/FieldsChunkSerializer.h>

namespace simu5g {

/**
 * Converts between GtpUserMsg and binary (network byte order) packet.
 */
class GtpUserMsgSerializer : public inet::FieldsChunkSerializer
{
  protected:
    void serialize(inet::MemoryOutputStream& stream, const inet::Ptr<const inet::Chunk>& chunk) const override;
    const inet::Ptr<inet::Chunk> deserialize(inet::MemoryInputStream& stream) const override;
};

} //namespace

#endif // ifndef __INET_GTPUSERMSGSERIALIZER_H

