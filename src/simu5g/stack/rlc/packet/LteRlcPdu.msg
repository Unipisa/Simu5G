//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.common.packet.Packet;
import simu5g.stack.rlc.LteRlcDefs;

cplusplus {{
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
}}

class size_t {
    @existingClass; // size_t
    @primitive;
    @fromString(string2uint64($));
    @toString(uint642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<size_t>($.intValue()));
}

namespace simu5g;

class LteRlcDataPdu extends inet::FieldsChunk
{
    chunkLength = inet::b(1);

    inet::Packet* sdu[] @sizeGetter(getNumSdu) @owned;
    size_t sduSize[];

    // Sequence number of the PDU
    unsigned int pduSequenceNumber = 0;

    // define the segmentation info for the PDU
    FramingInfo framingInfo;
}

cplusplus(LteRlcDataPdu) {{
  public:
    /**
     * pushSdu() gets ownership of the packet
     * and stores it inside the RLC SDU list
     * in back position
     *
     * @param pkt packet to store
     */
    virtual void pushSdu(inet::Packet *pkt);
    virtual void pushSdu(inet::Packet *pkt, int size);

    /**
     * popSdu() pops a packet from the front of
     * the SDU list and drops ownership before
     * returning it
     *
     * @return popped packet
     */
    virtual inet::Packet *popSdu(size_t& size);
}}

cplusplus(cc) {{

void LteRlcDataPdu::pushSdu(inet::Packet *pkt)
{
    pushSdu(pkt, pkt->getByteLength());
}

void LteRlcDataPdu::pushSdu(inet::Packet *pkt, int size)
{
    // Clear tags to make sure they are not "tunnelled" over to the receiver node and "accidentally"
    // used there -- with the exception of PdcpTrackingTag, as RLC-UM (UmRxEntity.cc) still refers to it.
    auto pdcpTag = pkt->removeTagIfPresent<PdcpTrackingTag>();
    pkt->clearTags();
    if (pdcpTag != nullptr)
        *pkt->addTag<PdcpTrackingTag>() = *pdcpTag;

    appendSdu(pkt);
    appendSduSize(size);
}

inet::Packet *LteRlcDataPdu::popSdu(size_t& size)
{
    auto pkt = removeSdu(0);
    eraseSdu(0);

    size = getSduSize(0);
    eraseSduSize(0);
    return pkt;
}

}}

//
// Lte RLC UM Data Pdu
//
// Define additional fields for UM PDU
//
class LteRlcUmDataPdu extends LteRlcDataPdu
{
    chunkLength = inet::B(RLC_HEADER_UM);
};

//
// Lte RLC AM Pdu.
//
class LteRlcAmPdu extends inet::FieldsChunk
{
    chunkLength = inet::B(1);
    unsigned int totalFragments;    // Total number of fragments
    unsigned int snoFragment;        // Sequence number of this fragment
    unsigned int snoMainPacket;        // ID of packet (sequence number)

    unsigned short amType enum(LteAmType);
    unsigned short txNumber;
    int firstSn;
    int lastSn;
    bool bitmap[] @custom;
}

cplusplus(LteRlcAmPdu) {{
  protected:
    // bitmap[] custom data
    std::vector<bool> bitmap_;

  public:
    // bitmap[] custom functions
    virtual size_t getBitmapArraySize() const { return bitmap_.size(); }
    virtual void setBitmapArraySize(size_t size) { bitmap_.resize(size); }
    virtual bool getBitmap(size_t k) const { return bitmap_.at(k); }
    virtual void setBitmap(size_t k, bool bitmap_var) { bitmap_[k] = bitmap_var; }
    virtual void appendBitmap(bool bitmap) { throw cRuntimeError("Method not implemented"); }
    virtual void insertBitmap(size_t k, bool bitmap) { throw cRuntimeError("Method not implemented"); }
    virtual void eraseBitmap(size_t k) { throw cRuntimeError("Method not implemented"); }

    // bitmap[] direct access functions
    virtual const std::vector<bool>& getBitmapVec() const { return bitmap_; }
    void setBitmapVec(const std::vector<bool>& bitmap_vec) { handleChange(); bitmap_ = bitmap_vec; }

    // sequence check functions
    virtual bool isWhole() const { return firstSn == lastSn; }
    virtual bool isFirst() const { return firstSn == snoFragment; }
    virtual bool isMiddle() const { return !isFirst() && !isLast(); }
    virtual bool isLast() const { return lastSn == snoFragment; }
}}

cplusplus(LteRlcAmPdu::copy) {{
    bitmap_ = other.bitmap_;
}}

cplusplus(LteRlcAmPdu::parsimPack) {{
    doParsimPacking(b, this->bitmap_);
}}

cplusplus(LteRlcAmPdu::parsimUnpack) {{
    // doParsimUnpacking(b, this->bitmap_);
    {
        int n;
        doParsimUnpacking(b, n);
        bitmap_.resize(n);
        bool bit;
        for (int i = 0; i < n; i++) {
            doParsimUnpacking(b, bit);
            bitmap_[i] = bit;
        }
    }
}}
