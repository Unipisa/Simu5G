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

#ifndef _NTNNRRLCAMRXENTITY_H_
#define _NTNNRRLCAMRXENTITY_H_

#include "simu5g/stack/rlc/am/NrRlcAmRxEntity.h"

namespace simu5g {

//
// Receiving side of an RLC AM entity on a bearer carried over a transparent NTN path.
//
// It differs from ~NrRlcAmRxEntity in two values, t_Reassembly and t_StatusProhibit, which it
// derives from the round-trip delay the Binder computes for this cell. Reassembly and status
// reporting themselves are inherited unchanged.
//
class NtnNrRlcAmRxEntity : public NrRlcAmRxEntity
{
  protected:
    void initMode() override;
};

} // namespace simu5g

#endif
