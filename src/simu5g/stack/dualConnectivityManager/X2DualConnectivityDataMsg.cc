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

#include "stack/dualConnectivityManager/X2DualConnectivityDataMsg.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

//Register_Serializer(X2DualConnectivityDataMsg, LteX2MsgSerializer);

X2DualConnectivityDataMsg::X2DualConnectivityDataMsg() :
    LteX2Message() {
    type_ = X2_DUALCONNECTIVITY_DATA_MSG;
}

X2DualConnectivityDataMsg::X2DualConnectivityDataMsg(const X2DualConnectivityDataMsg& other) :
    LteX2Message() {
    operator=(other);
}

X2DualConnectivityDataMsg& X2DualConnectivityDataMsg::operator=(const X2DualConnectivityDataMsg& other) {
    if (&other == this)
        return *this;
    LteX2Message::operator=(other);
    return *this;
}

X2DualConnectivityDataMsg *X2DualConnectivityDataMsg::dup() const {
    return new X2DualConnectivityDataMsg(*this);
}


} //namespace

