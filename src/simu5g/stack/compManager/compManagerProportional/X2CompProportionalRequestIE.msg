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

import simu5g.stack.compManager.X2CompRequestIE;

namespace simu5g;

//
// X2CompProportionalRequestIE
//
class X2CompProportionalRequestIE extends X2CompRequestIE
{
    // number of "ideally" required blocks to satisfy the queue of all UEs
    uint32_t numBlocks;

    type = COMP_PROP_REQUEST_IE;
    length = sizeof(uint32_t);
};
