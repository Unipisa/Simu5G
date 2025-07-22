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

import simu5g.x2.packet.X2InformationElement;

namespace simu5g;

class X2CompReplyIE extends X2InformationElement
{
    type = COMP_REPLY_IE;
    length = 0;
};
