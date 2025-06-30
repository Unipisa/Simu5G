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

#ifndef _LTE_X2DUALCONNECTIVITYDATAMSG_H_
#define _LTE_X2DUALCONNECTIVITYDATAMSG_H_

#include "x2/packet/LteX2Message.h"
#include "common/LteCommon.h"
#include "common/LteControlInfo_m.h"

namespace simu5g {

/**
 * @class X2DualConnectivityDataMsg
 *
 * Class derived from LteX2Message
 * It defines the message that encapsulates PDCP PDUs to be exchanged between Dual Connectivity managers.
 */
class X2DualConnectivityDataMsg : public LteX2Message
{
  public:

    X2DualConnectivityDataMsg();

    X2DualConnectivityDataMsg(const X2DualConnectivityDataMsg& other);

    X2DualConnectivityDataMsg& operator=(const X2DualConnectivityDataMsg& other);

    X2DualConnectivityDataMsg *dup() const override;

};

//Register_Class(X2DualConnectivityDataMsg);

} //namespace

#endif

