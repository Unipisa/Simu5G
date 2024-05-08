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
#include "stack/rlc/LteRlcDefs.h"
}}

namespace simu5g;

enum LteAmType;

struct RlcFragDesc { @existingClass; }
struct RlcWindowDesc { @existingClass; }

struct MrwDesc { @existingClass; }
enum RlcAmTimerType;

struct RlcUmRxWindowDesc { @existingClass; }
enum RlcUmTimerType;