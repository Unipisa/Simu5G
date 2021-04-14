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

#ifndef _LTE_X2COMPREPLYIE_H_
#define _LTE_X2COMPREPLYIE_H_

#include "x2/packet/X2InformationElement.h"


//
// X2CompReplyIE
// Base class for CoMP reply messages
//
class X2CompReplyIE : public X2InformationElement
{
  protected:

  public:
    X2CompReplyIE()
    {
        type_ = COMP_REPLY_IE;
        length_ = 0;
    }
    X2CompReplyIE(const X2CompReplyIE& other) :
        X2InformationElement()
    {
        operator=(other);
    }

    X2CompReplyIE& operator=(const X2CompReplyIE& other)
    {
        if (&other == this)
            return *this;
        X2InformationElement::operator=(other);
        return *this;
    }
    virtual X2CompReplyIE *dup() const
    {
        return new X2CompReplyIE(*this);
    }
    virtual ~X2CompReplyIE() {}
};

#endif
