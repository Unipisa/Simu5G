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

#include "simu5g/stack/pdcp/packet/RohcHeader.h"

#include "inet/common/packet/chunk/EmptyChunk.h"
#include "inet/common/packet/chunk/SliceChunk.h"

using namespace omnetpp;
using namespace inet;

namespace simu5g {

Register_Class(RohcHeader);

RohcHeader::RohcHeader() :
    inet::Chunk(),
    chunk(nullptr),
    length(inet::b(-1))
{
}

RohcHeader::RohcHeader(const inet::Ptr<inet::Chunk>& chunk, inet::b length) :
    inet::Chunk(),
    chunk(chunk),
    length(length)
{
    CHUNK_CHECK_USAGE(chunk->isImmutable(), "chunk is mutable");
}

void RohcHeader::parsimPack(omnetpp::cCommBuffer *buffer) const
{
    inet::Chunk::parsimPack(buffer);
    buffer->packObject(chunk.get());
    buffer->pack(length.get());
}

void RohcHeader::parsimUnpack(omnetpp::cCommBuffer *buffer)
{
    inet::Chunk::parsimUnpack(buffer);
    chunk = inet::check_and_cast<inet::Chunk *>(buffer->unpackObject())->shared_from_this();
    int64_t lengthValue;
    buffer->unpack(lengthValue);
    length = inet::b(lengthValue);
}

void RohcHeader::forEachChild(omnetpp::cVisitor *v)
{
    inet::Chunk::forEachChild(v);
    if (chunk)
        v->visit(const_cast<inet::Chunk *>(chunk.get()));
}

void RohcHeader::setChunk(const inet::Ptr<inet::Chunk>& chunk)
{
    CHUNK_CHECK_USAGE(chunk->isImmutable(), "chunk is mutable"); this->chunk = chunk;
}

bool RohcHeader::containsSameData(const inet::Chunk& other) const
{
    if (&other == this)
        return true;
    else if (!inet::Chunk::containsSameData(other))
        return false;
    else {
        auto otherRohc = static_cast<const RohcHeader *>(&other);
        return (!chunk && !otherRohc->chunk) ||
               (chunk && otherRohc->chunk && chunk->containsSameData(*otherRohc->chunk.get()));
    }
}

const inet::Ptr<inet::Chunk> RohcHeader::peekUnchecked(PeekPredicate predicate, PeekConverter converter, const Iterator& iterator, inet::b length, int flags) const
{
    inet::b chunkLength = getChunkLength();
    CHUNK_CHECK_USAGE(inet::b(0) <= iterator.getPosition() && iterator.getPosition() <= chunkLength, "iterator is out of range");
    // 1. peeking an empty part returns nullptr
    if (length == inet::b(0) || (iterator.getPosition() == chunkLength && length < inet::b(0))) {
        if (predicate == nullptr || predicate(nullptr))
            return inet::EmptyChunk::getEmptyChunk(flags);
    }
    // 2. peeking the whole part returns this chunk
    if (iterator.getPosition() == inet::b(0) && (-length >= chunkLength || length == chunkLength)) {
        auto result = const_cast<RohcHeader *>(this)->shared_from_this();
        if (predicate == nullptr || predicate(result))
            return result;
    }
    // 3. peeking without conversion returns a SliceChunk
    if (converter == nullptr)
        return peekConverted<inet::SliceChunk>(iterator, length, flags);
    // 4. peeking with conversion
    return converter(const_cast<RohcHeader *>(this)->shared_from_this(), iterator, length, flags);
}

void RohcHeader::setLength(inet::b length)
{
    handleChange();
    this->length = length;
}

std::ostream& RohcHeader::printFieldsToStream(std::ostream& stream, int level, int evFlags) const
{
    if (level <= PRINT_LEVEL_DETAIL) {
        stream << EV_FIELD(chunk, printFieldToString(chunk.get(), level + 1, evFlags));
        stream << EV_FIELD(length);
    }
    return stream;
}

} // namespace simu5g
