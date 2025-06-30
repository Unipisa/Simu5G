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

#ifndef _LTE_X2COMPREQUESTIE_H_
#define _LTE_X2COMPREQUESTIE_H_

#include "x2/packet/X2InformationElement.h"

namespace simu5g {

//
// X2CompRequestIE
// Base class for CoMP request messages
//
class X2CompRequestIE : public X2InformationElement
{
  public:
    X2CompRequestIE()
    {
        type_ = COMP_REQUEST_IE;
        length_ = 0;
    }

    X2CompRequestIE(const X2CompRequestIE& other) :
        X2InformationElement()
    {
        operator=(other);
    }

    X2CompRequestIE& operator=(const X2CompRequestIE& other)
    {
        if (&other == this)
            return *this;
        X2InformationElement::operator=(other);
        return *this;
    }

    X2CompRequestIE *dup() const override
    {
        return new X2CompRequestIE(*this);
    }

};

} //namespace

#endif

