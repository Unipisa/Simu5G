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

#ifndef _LTE_X2COMPPROPORTIONALREPLYIE_H_
#define _LTE_X2COMPPROPORTIONALREPLYIE_H_

#include "stack/compManager/X2CompReplyIE.h"

namespace simu5g {

enum CompRbStatus
{
    AVAILABLE_RB, NOT_AVAILABLE_RB
};

//
// X2CompProportionalReplyIE
//
class X2CompProportionalReplyIE : public X2CompReplyIE
{
  protected:

    // For each block, it indicates whether the eNB can use that block
    std::vector<CompRbStatus> allowedBlocksMap_;

  public:
    X2CompProportionalReplyIE()
    {
        length_ = sizeof(uint32_t);    // Required to store the length of the map
        type_ = COMP_PROP_REPLY_IE;
    }

    X2CompProportionalReplyIE(const X2CompProportionalReplyIE& other) :
        X2CompReplyIE()
    {
        operator=(other);
    }

    X2CompProportionalReplyIE& operator=(const X2CompProportionalReplyIE& other)
    {
        if (&other == this)
            return *this;
        allowedBlocksMap_ = other.allowedBlocksMap_;
        X2InformationElement::operator=(other);
        return *this;
    }

    X2CompProportionalReplyIE *dup() const override
    {
        return new X2CompProportionalReplyIE(*this);
    }


    // Getter/setter methods
    void setAllowedBlocksMap(std::vector<CompRbStatus>& map)
    {
        allowedBlocksMap_ = map;
        // Number of bytes of the map when being serialized
        length_ += allowedBlocksMap_.size() * sizeof(uint8_t);
    }

    std::vector<CompRbStatus>& getAllowedBlocksMap() { return allowedBlocksMap_; }
};

} //namespace

#endif

