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

cplusplus {{
#include <vector>
}}

import simu5g.stack.compManager.X2CompReplyIE;

namespace simu5g;

enum CompRbStatus
{
    AVAILABLE_RB = 0;
    NOT_AVAILABLE_RB = 1;
};

//
// X2CompProportionalReplyIE
//
class X2CompProportionalReplyIE extends X2CompReplyIE
{
    length = sizeof(uint32_t);    // Required to store the length of the map
    type = COMP_PROP_REPLY_IE;
    CompRbStatus allowedBlocksMap[];
    //std::vector<CompRbStatus> allowedBlocksMap_;
}

cplusplus(X2CompProportionalReplyIE) {{
  public:
    void setAllowedBlocksMap(std::vector<CompRbStatus>& map);
    const std::vector<CompRbStatus> getAllowedBlocksMap() const;
}}
