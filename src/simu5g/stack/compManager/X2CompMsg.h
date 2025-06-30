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

#ifndef _LTE_X2COMPMSG_H_
#define _LTE_X2COMPMSG_H_

#include "x2/packet/LteX2Message.h"
#include "x2/packet/LteX2MsgSerializer.h"
#include "common/LteCommon.h"

namespace simu5g {

/**
 * @class X2CompMsg
 *
 * Class derived from LteX2Message
 * It defines the message exchanged between CoMP managers
 */
class X2CompMsg : public LteX2Message
{

  public:

    X2CompMsg();

    X2CompMsg(const X2CompMsg& other);

    X2CompMsg& operator=(const X2CompMsg& other);

    X2CompMsg *dup() const override;

};

} //namespace

#endif

