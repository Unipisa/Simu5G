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

#ifndef LTERLCPDU_H_
#define LTERLCPDU_H_

#include "stack/rlc/packet/LteRlcPdu_m.h"
#include "common/LteControlInfo.h"

namespace simu5g {

class LteRlcPdu : public LteRlcPdu_Base
{
  private:
    void copy(const LteRlcPdu& other)
    {
        this->totalFragments = other.totalFragments;
        this->snoFragment = other.snoFragment;
        this->snoMainPacket = other.snoMainPacket;
    }

  public:
    LteRlcPdu(const char *name = nullptr, int kind = 0) : LteRlcPdu_Base(name, kind) {}
    LteRlcPdu(const LteRlcPdu& other) : LteRlcPdu_Base(other) { copy(other); }

    LteRlcPdu& operator=(const LteRlcPdu& other)
    {
        if (this == &other)
            return *this;
        LteRlcPdu_Base::operator=(other);
        copy(other);
        return *this;
    }

    virtual LteRlcPdu *dup() const { return new LteRlcPdu(*this); }
    virtual ~LteRlcPdu() {
        cObject *ctrlInfo = removeControlInfo();
        if (ctrlInfo != nullptr) {
            delete ctrlInfo;
        }
    }

};

Register_Class(LteRlcPdu);

} //namespace

#endif /* LTERLCPDU_H_ */

