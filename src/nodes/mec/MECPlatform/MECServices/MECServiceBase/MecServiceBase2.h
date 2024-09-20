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

#ifndef __SIMU5G_MECSERVICEBASE2_H
#define __SIMU5G_MECSERVICEBASE2_H

#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"

namespace simu5g {

class MecServiceBase2 : public MecServiceBase
{
  protected:
    void initialize(int stage) override;
};

} //namespace

#endif // __SIMU5G_MECSERVICEBASE2_H

