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

import simu5g.stack.rlc.LteRlcDefs;
import simu5g.stack.rlc.packet.LteRlcPdu;

namespace simu5g;

class LteRlcAmPdu extends LteRlcPdu
{
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
