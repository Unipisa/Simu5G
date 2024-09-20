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

#ifndef _LTE_X2HANDOVERCOMMAND_H_
#define _LTE_X2HANDOVERCOMMAND_H_

#include "x2/packet/X2InformationElement.h"

namespace simu5g {

//
// X2HandoverCommandIE
//
class X2HandoverCommandIE : public X2InformationElement
{
  protected:

    bool startHandover_ = false;
    MacNodeId ueId_ = NODEID_NONE;      // ID of the user performing the handover

  public:
    X2HandoverCommandIE()
    {
        type_ = X2_HANDOVER_CMD_IE;
        length_ = sizeof(MacNodeId) + sizeof(uint8_t);
    }

    X2HandoverCommandIE(const X2HandoverCommandIE& other) :
        X2InformationElement()
    {
        operator=(other);
    }

    X2HandoverCommandIE& operator=(const X2HandoverCommandIE& other)
    {
        if (&other == this)
            return *this;
        startHandover_ = other.startHandover_;
        ueId_ = other.ueId_;
        X2InformationElement::operator=(other);
        return *this;
    }

    X2HandoverCommandIE *dup() const override
    {
        return new X2HandoverCommandIE(*this);
    }


    // getter/setter methods
    void setStartHandover() { startHandover_ = true; }
    bool isStartHandover() { return startHandover_; }
    void setUeId(MacNodeId ueId) { ueId_ = ueId; }
    MacNodeId getUeId() { return ueId_; }
};

} //namespace

#endif

