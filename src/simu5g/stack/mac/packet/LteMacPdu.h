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

#ifndef _LTE_LTEMACPDU_H_
#define _LTE_LTEMACPDU_H_

#include "simu5g/stack/mac/packet/LteMacPdu_m.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"

namespace simu5g {

/**
 * @class LteMacPdu
 * @brief Lte MAC Pdu
 *
 * Class derived from base class contained
 * in msg declaration: adds the SDU and control elements list
 * TODO: Add Control Elements
 */
class LteMacPdu : public LteMacPdu_Base
{
  protected:
    /// ID of the MAC PDU: incrementally set according to the static variable numMacPdus
    int64_t macPduId_;

private:
    void copy(const LteMacPdu &other);

  public:
    /**
     * Constructor
     */
    LteMacPdu();

    /**
     * Copy constructors
     */
    LteMacPdu(const LteMacPdu& other) : LteMacPdu_Base(other) { copy(other); }

    /**
     * Destructor
     */
    ~LteMacPdu() override;

    LteMacPdu& operator=(const LteMacPdu &other);

    LteMacPdu *dup() const override { return new LteMacPdu(*this); }

    /**
     * info() prints a one line description of this object
     */
    std::string str() const override;

    void forEachChild(cVisitor *v) override;

    void parsimPack(omnetpp::cCommBuffer *b) const override;
    void parsimUnpack(omnetpp::cCommBuffer *b) override;

    long getId() const { return macPduId_; }

    void setHeaderLength(unsigned int headerLength) override {
        LteMacPdu_Base::setHeaderLength(headerLength);
        this->setChunkLength(b(getBitLength()));
    }
};

} //namespace

#endif
