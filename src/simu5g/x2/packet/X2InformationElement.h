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

#ifndef _LTE_X2INFORMATIONELEMENT_H_
#define _LTE_X2INFORMATIONELEMENT_H_

#include "x2/packet/LteX2Message_m.h"
#include "common/LteCommon.h"

namespace simu5g {

// add here new IE types
enum X2InformationElementType
{
    COMP_REQUEST_IE,         // CoMP slave -> master
    COMP_REPLY_IE,           // CoMP master -> slave
    COMP_PROP_REQUEST_IE,    // CoMP slave -> master (for compManagerProportional, with RB value)
    COMP_PROP_REPLY_IE,      // CoMP master -> slave (for compManagerProportional, with map)
    X2_HANDOVER_CMD_IE       // HO command source eNB -> target eNB
};

//
// X2 Information Element
//
// Extend this class to define new Information Elements
//
class X2InformationElement : public X2InformationElement_Base
{
  protected:
    X2InformationElementType type_;
    int64_t length_;

  public:

    X2InformationElement() : X2InformationElement_Base() {}
    X2InformationElement(X2InformationElementType type) : X2InformationElement_Base(), type_(type) { }


    X2InformationElement(const X2InformationElement& other) : X2InformationElement_Base()
    {
        operator=(other);
    }

    X2InformationElement& operator=(const X2InformationElement& other)
    {
        if (&other == this)
            return *this;
        X2InformationElement_Base::operator=(other);
        type_ = other.type_;
        length_ = other.length_;
        return *this;
    }

    virtual X2InformationElement *dup() const
    {
        return new X2InformationElement(*this);
    }

    // field getter/setter methods
    virtual X2InformationElementType getType() const { return type_; }
    // return length (in bytes)
    virtual int64_t getLength() const { return length_; }
};

} //namespace

#endif

