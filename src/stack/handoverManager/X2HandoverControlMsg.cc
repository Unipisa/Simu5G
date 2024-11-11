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

#include "X2HandoverControlMsg.h"

#include <inet/common/packet/serializer/ChunkSerializerRegistry.h>

#include "x2/packet/LteX2MsgSerializer.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Register_Serializer(X2HandoverControlMsg, LteX2MsgSerializer);

X2HandoverControlMsg::X2HandoverControlMsg() :
    LteX2Message() {
    type_ = X2_HANDOVER_CONTROL_MSG;
}

X2HandoverControlMsg::X2HandoverControlMsg(const X2HandoverControlMsg& other) :
    LteX2Message() {
    operator=(other);
}

X2HandoverControlMsg& X2HandoverControlMsg::operator=(const X2HandoverControlMsg& other) {
    if (&other == this)
        return *this;
    LteX2Message::operator=(other);
    return *this;
}

X2HandoverControlMsg *X2HandoverControlMsg::dup() const {
    return new X2HandoverControlMsg(*this);
}


} //namespace

