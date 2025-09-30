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

#ifndef __SIMU5G_ROHCHEADER_H
#define __SIMU5G_ROHCHEADER_H

#include "inet/common/packet/chunk/Chunk.h"

namespace simu5g {

/**
 * This class represents ROHC (RObust Header Compression) compressed data of another chunk.
 * The original data is specified with another chunk, and the ROHC header stores information
 * about the original sizes of the compressed headers to allow proper decompression.
 *
 * ROHC is modeled by reducing the compressed headers to a lower size as indicated by the
 * headerCompressedSize_ parameter. The additional ROHC header allows to restore the
 * compressed headers to their full size when decompressing.
 */
class RohcHeader : public inet::Chunk
{
    friend class RohcHeaderDescriptor;

  protected:
    /**
     * The original uncompressed header chunks, or nullptr if not yet specified.
     */
    inet::Ptr<inet::Chunk> chunk;
    /**
     * The compressed length measured in bits, or -1 if not yet specified.
     */
    inet::b length;

  protected:
    inet::Chunk *_getChunk() const { return chunk.get(); } // only for class descriptor

    virtual const inet::Ptr<inet::Chunk> peekUnchecked(PeekPredicate predicate, PeekConverter converter, const Iterator& iterator, inet::b length, int flags) const override;

  public:
    /** @name Constructors, destructors and duplication related functions */
    //@{
    RohcHeader();
    RohcHeader(const RohcHeader& other) = default;
    RohcHeader(const inet::Ptr<inet::Chunk>& chunk, inet::b length);

    virtual RohcHeader *dup() const override { return new RohcHeader(*this); }
    virtual const inet::Ptr<inet::Chunk> dupShared() const override { return inet::makeShared<RohcHeader>(*this); }

    virtual void parsimPack(omnetpp::cCommBuffer *buffer) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *buffer) override;
    //@}

    virtual void forEachChild(omnetpp::cVisitor *v) override;

    /** @name Field accessor functions */
    //@{
    const inet::Ptr<inet::Chunk>& getChunk() const { return chunk; }
    void setChunk(const inet::Ptr<inet::Chunk>& chunk);

    inet::b getLength() const { return length; }
    void setLength(inet::b length);
    //@}

    /** @name Overridden flag functions */
    //@{
    virtual bool isMutable() const override { return inet::Chunk::isMutable() || (chunk && chunk->isMutable()); }
    virtual bool isImmutable() const override { return inet::Chunk::isImmutable() && (!chunk || chunk->isImmutable()); }

    virtual bool isComplete() const override { return inet::Chunk::isComplete() && (!chunk || chunk->isComplete()); }
    virtual bool isIncomplete() const override { return inet::Chunk::isIncomplete() || (chunk && chunk->isIncomplete()); }

    virtual bool isCorrect() const override { return inet::Chunk::isCorrect() && (!chunk || chunk->isCorrect()); }
    virtual bool isIncorrect() const override { return inet::Chunk::isIncorrect() || (chunk && chunk->isIncorrect()); }

    virtual bool isProperlyRepresented() const override { return inet::Chunk::isProperlyRepresented() && (!chunk || chunk->isProperlyRepresented()); }
    virtual bool isImproperlyRepresented() const override { return inet::Chunk::isImproperlyRepresented() || (chunk && chunk->isImproperlyRepresented()); }
    //@}

    /** @name Overridden chunk functions */
    //@{
    virtual ChunkType getChunkType() const override { return CT_ENCRYPTED; } // Use CT_SEQUENCE as it's closest to our use case
    virtual inet::b getChunkLength() const override { CHUNK_CHECK_IMPLEMENTATION(length >= inet::b(0)); return length; }

    virtual bool containsSameData(const inet::Chunk& other) const override;

    virtual std::ostream& printFieldsToStream(std::ostream& stream, int level, int evFlags = 0) const override;
    //@}
};

} // namespace simu5g

#endif
