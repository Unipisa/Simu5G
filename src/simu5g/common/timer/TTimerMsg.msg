//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

import simu5g.common.LteCommon;

namespace simu5g;

enum TTimerType {
    TTSIMPLE = 0;
    TTMULTI = 1;
};

packet TTimerMsg
{
    unsigned int timerId;
    unsigned short type enum(TTimerType);
}
