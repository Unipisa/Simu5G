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

#ifndef _NTNNRRLCUMRXENTITY_H_
#define _NTNNRRLCUMRXENTITY_H_

#include "simu5g/stack/rlc/um/NrRlcUmRxEntity.h"

namespace simu5g {

//
// Receiving side of an RLC UM entity on a bearer carried over a transparent NTN path.
//
// It differs from ~NrRlcUmRxEntity in exactly one value, t_Reassembly, which it derives from the
// round-trip delay the Binder computes for this cell. Reassembly itself is inherited unchanged.
//
class NtnNrRlcUmRxEntity : public NrRlcUmRxEntity
{
  protected:
    void initMode(LteMacBase *mac) override;
};

} // namespace simu5g

#endif
