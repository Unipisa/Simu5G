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

#ifndef _NRMACGNB_H_
#define _NRMACGNB_H_

#include "stack/mac/LteMacEnbD2D.h"

namespace simu5g {

class NRMacGnb : public LteMacEnbD2D
{
  public:

    NRMacGnb();

    /**
     * Reads MAC parameters and performs initialization.
     */
    void initialize(int stage) override;

};

} //namespace

#endif

