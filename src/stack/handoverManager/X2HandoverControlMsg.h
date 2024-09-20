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

#ifndef _LTE_X2HANDOVERCONTROLMSG_H_
#define _LTE_X2HANDOVERCONTROLMSG_H_

#include "x2/packet/LteX2Message.h"
#include "common/LteCommon.h"

namespace simu5g {

/**
 * @class X2HandoverControlMsg
 *
 * Class derived from LteX2Message
 * It defines the messages exchanged between Handover managers
 */
class X2HandoverControlMsg : public LteX2Message
{

  public:

    X2HandoverControlMsg();

    X2HandoverControlMsg(const X2HandoverControlMsg& other);

    X2HandoverControlMsg& operator=(const X2HandoverControlMsg& other);

    X2HandoverControlMsg *dup() const override;

};

} //namespace

#endif

