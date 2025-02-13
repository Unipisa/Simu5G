//
// Copyright (C) 2020 OpenSim Ltd.
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
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef _HTTPREQUESTMESSAGESERIALIZER_H_
#define _HTTPREQUESTMESSAGESERIALIZER_H_

#include <inet/common/packet/serializer/FieldsChunkSerializer.h>

namespace simu5g {

using namespace inet;

/**
 * Converts between ApplicationPacket and binary (network byte order) application packet.
 */
class HttpRequestMessageSerializer : public FieldsChunkSerializer
{
  protected:
    void serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const override;
    const Ptr<Chunk> deserialize(MemoryInputStream& stream) const override;
};

} //namespace

#endif

