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

import  stack.rlc.LteRlcDefs;
import  stack.rlc.packet.LteRlcPdu;

namespace simu5g;

// Rlc Am Control fields for AM control PDUs
//class LteRlcAmControlInfo
//{
//     @customize(true);
//     unsigned int firstSn;
//     unsigned int lastSn;
//     abstract bool bitmap[];
//}

class LteRlcAmPdu extends LteRlcPdu
{
    @customize(true);
    unsigned short amType enum(LteAmType);
    unsigned short txNumber;
    int firstSn;
    int lastSn;
    abstract bool bitmap[];
}