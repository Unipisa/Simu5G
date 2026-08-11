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

#ifndef _NTNNRMACUE_H_
#define _NTNNRMACUE_H_

#include "simu5g/stack/mac/NrMacUe.h"

namespace simu5g {

//
// NR MAC for a UE served through a transparent NTN path. It re-dimensions the
// random-access and buffer-status-report counters for satellite round-trip
// delay and changes nothing else; see NtnNrMacUe.ned for the derivation of the
// values from TR 38.821 and TS 38.331.
//
// The counters themselves are inherited slot counts. This class only converts
// the NED parameters, which are expressed in time, into slot counts against the
// slot duration this UE actually runs at, and overwrites the inherited reload
// values. Nothing in the random-access path is overridden: checkRAC() and
// macHandleRac() only ever read these members, so replacing them after
// initialisation is sufficient and avoids duplicating an unfactored 75-line
// function.
//
class NtnNrMacUe : public NrMacUe
{
  protected:
    void initialize(int stage) override;
};

} //namespace

#endif
