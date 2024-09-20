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

#ifndef _LTE_X2HANDOVERDATAMSG_H_
#define _LTE_X2HANDOVERDATAMSG_H_

#include "x2/packet/LteX2Message.h"
#include "common/LteCommon.h"

namespace simu5g {

/**
 * @class X2HandoverDataMsg
 *
 * Class derived from LteX2Message
 * It defines the message that encapsulates the datagram to be exchanged between Handover managers
 */
class X2HandoverDataMsg : public LteX2Message
{

  public:

    X2HandoverDataMsg() :
        LteX2Message()
    {
        type_ = X2_HANDOVER_DATA_MSG;
    }

    X2HandoverDataMsg(const X2HandoverDataMsg& other) : LteX2Message() { operator=(other); }

    X2HandoverDataMsg& operator=(const X2HandoverDataMsg& other)
    {
        if (&other == this)
            return *this;
        LteX2Message::operator=(other);
        return *this;
    }

    X2HandoverDataMsg *dup() const override { return new X2HandoverDataMsg(*this); }

};

} //namespace

#endif

