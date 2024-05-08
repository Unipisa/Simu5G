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

#include "stack/rlc/packet/LteRlcDataPdu.h"

namespace simu5g {

void LteRlcDataPdu::setPduSequenceNumber(unsigned int sno)
{
    pduSequenceNumber_ = sno;
}

unsigned int LteRlcDataPdu::getPduSequenceNumber() const
{
    return pduSequenceNumber_;
}

void LteRlcDataPdu::setFramingInfo(FramingInfo fi)
{
    fi_ = fi;
}

FramingInfo LteRlcDataPdu::getFramingInfo() const
{
    return fi_;
}

} //namespace

